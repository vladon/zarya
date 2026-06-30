#!/usr/bin/env python3
"""Verify Zarya release artifacts (checksums, manifest, optional signatures)."""

from __future__ import annotations

import argparse
import glob
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import (  # noqa: E402
    FORBIDDEN_ARTIFACT_NAMES,
    INSTALLER_DOC_FILES,
    PUBLIC_BETA_DOC_FILES,
    RC_DOC_FILES,
    STABLE_RELEASE_DOC_FILES,
    UPDATER_DOC_FILES,
    STABLE_DOC_FILES,
    artifact_content_root,
    artifact_gui_candidates,
    extract_tar_gz,
    extract_zip,
    read_cmake_version,
    run_version_check,
    sha256_file,
)

INSTALLER_SKELETON_PATHS = (
    "packaging/windows/wix/Product.wxs",
    "packaging/windows/wix/Registry.wxs",
    "packaging/windows/wix/Directories.wxs",
    "scripts/package-windows-msi.ps1",
    "docs/installer/windows-msi-poc.md",
    "packaging/linux/deb/DEBIAN/control.in",
    "packaging/linux/rpm/zarya.spec.in",
    "packaging/metadata/app-id.txt",
)

PRODUCTION_INSTALLER_CLAIMS = (
    "production msi",
    "production pkg",
    "production deb",
    "replaces portable beta artifacts",
)

PRODUCTION_SELF_UPDATE_CLAIMS = (
    "production self-update",
    "automatic app replacement enabled",
    "silent auto-update",
)

ISSUE_TEMPLATE_FILES = (
    "bug_report.yml",
    "diagnostics_issue.yml",
    "feature_request.yml",
    "config_validation.yml",
    "experimental_tun_issue.yml",
    "security.md",
)


def find_staging_root(extracted: Path) -> Path:
    children = [p for p in extracted.iterdir() if p.name != "__MACOSX"]
    if len(children) == 1 and children[0].is_dir():
        return children[0]
    return extracted


def load_manifest(staging: Path) -> dict | None:
    manifest = staging / "release-manifest.json"
    if not manifest.is_file():
        manifest = next(staging.rglob("release-manifest.json"), None)
    if manifest is None or not manifest.is_file():
        return None
    return json.loads(manifest.read_text(encoding="utf-8"))


def checksum_for_artifact(output_dir: Path, artifact: Path) -> str | None:
    sidecar = output_dir / f"{artifact.name}.sha256"
    if sidecar.is_file():
        line = sidecar.read_text(encoding="utf-8").strip().splitlines()[0]
        return line.split()[0]
    sums = output_dir / "SHA256SUMS.txt"
    if sums.is_file():
        for line in sums.read_text(encoding="utf-8").splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[-1] == artifact.name:
                return parts[0]
    return None


def verify_installer_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in INSTALLER_DOC_FILES:
        if not (staging / "docs" / "installer" / name).is_file():
            errors.append(f"missing installer doc: docs/installer/{name}")
    return errors


def verify_updater_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in UPDATER_DOC_FILES:
        if not (staging / "docs" / "updater" / name).is_file():
            errors.append(f"missing updater doc: docs/updater/{name}")
    return errors


def verify_stable_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in STABLE_DOC_FILES:
        if not (staging / "docs" / "stable" / name).is_file():
            errors.append(f"missing stable doc: docs/stable/{name}")
    return errors


def verify_stable_source(source_root: Path) -> list[str]:
    errors: list[str] = []
    required = (
        "src/features/FeatureGate.cpp",
        "docs/stable/stable-scope.md",
        "docs/stable/release-criteria.md",
    )
    for relative in required:
        if not (source_root / relative).is_file():
            errors.append(f"missing stable hardening source/doc: {relative}")
    return errors


def verify_updater_source(source_root: Path) -> list[str]:
    errors: list[str] = []
    required = (
        "src/updater/AppUpdateChecker.cpp",
        "src/updater/AppUpdatePlanner.cpp",
        "src/updater/AppVersion.cpp",
        "src/updater/runner/main.cpp",
        "scripts/generate-update-manifest.py",
        "docs/updater/README.md",
    )
    for relative in required:
        if not (source_root / relative).is_file():
            errors.append(f"missing updater source/doc: {relative}")
    return errors


def verify_no_production_self_update_claims(staging: Path) -> list[str]:
    errors: list[str] = []
    scan_paths = [staging / "RELEASE_NOTES.md", staging / "docs" / "updater" / "README.md"]
    for path in scan_paths:
        if not path.is_file():
            continue
        lowered = path.read_text(encoding="utf-8", errors="replace").lower()
        for phrase in PRODUCTION_SELF_UPDATE_CLAIMS:
            if phrase in lowered:
                errors.append(f"forbidden self-update claim in {path.name}: {phrase}")
    updater_readme = staging / "docs" / "updater" / "README.md"
    if updater_readme.is_file():
        text = updater_readme.read_text(encoding="utf-8", errors="replace").lower()
        if "portable" not in text or "installed" not in text:
            errors.append("updater README must describe portable vs installed update scope")
    return errors


def verify_installer_skeletons(source_root: Path) -> list[str]:
    errors: list[str] = []
    for relative in INSTALLER_SKELETON_PATHS:
        if not (source_root / relative).is_file():
            errors.append(f"missing installer skeleton: {relative}")
    if not (source_root / "src" / "packaging" / "InstallationMode.cpp").is_file():
        errors.append("missing InstallationMode implementation")
    return errors


def verify_no_production_installer_claims(staging: Path) -> list[str]:
    errors: list[str] = []
    scan_paths = [staging / "RELEASE_NOTES.md", staging / "docs" / "installer" / "README.md"]
    for path in scan_paths:
        if not path.is_file():
            continue
        lowered = path.read_text(encoding="utf-8", errors="replace").lower()
        if "no production msi" in lowered or "no production-signed msi" in lowered:
            continue
        for phrase in PRODUCTION_INSTALLER_CLAIMS:
            if phrase in lowered:
                errors.append(f"forbidden production installer claim in {path.name}: {phrase}")
    installer_readme = staging / "docs" / "installer" / "README.md"
    if installer_readme.is_file():
        text = installer_readme.read_text(encoding="utf-8", errors="replace")
        lowered = text.lower()
        if (
            "no production msi" not in lowered
            and "no production-signed msi" not in lowered
        ):
            errors.append("installer README must state no production installer yet")
    return errors


def verify_public_beta_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in PUBLIC_BETA_DOC_FILES:
        if not (staging / "docs" / "public-beta" / name).is_file():
            errors.append(f"missing public-beta doc: docs/public-beta/{name}")
    return errors


def verify_issue_templates(source_root: Path) -> list[str]:
    errors: list[str] = []
    for name in ISSUE_TEMPLATE_FILES:
        if not (source_root / ".github" / "ISSUE_TEMPLATE" / name).is_file():
            errors.append(f"missing issue template: .github/ISSUE_TEMPLATE/{name}")
    if not (source_root / ".github" / "pull_request_template.md").is_file():
        errors.append("missing .github/pull_request_template.md")
    return errors


def verify_rc_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in RC_DOC_FILES:
        if not (staging / "docs" / "rc" / name).is_file():
            errors.append(f"missing rc doc: docs/rc/{name}")
    known_issues = staging / "docs" / "rc" / "rc-known-issues.md"
    if known_issues.is_file():
        text = known_issues.read_text(encoding="utf-8", errors="replace").lower()
        if "msi" not in text or "portable" not in text:
            errors.append("rc-known-issues.md must mention MSI PoC and portable artifact")
    release_notes = staging / "docs" / "release-notes" / "0.36-rc1.md"
    if not release_notes.is_file():
        errors.append("missing release notes: docs/release-notes/0.36-rc1.md")
    return errors


def verify_rc_source(source_root: Path) -> list[str]:
    errors: list[str] = []
    required = (
        "src/features/FeaturePolicy.cpp",
        "src/storage/AppSettings.cpp",
        "scripts/audit-redaction.py",
        "docs/rc/rc-scope.md",
        "docs/rc/rc-blockers.md",
    )
    for relative in required:
        if not (source_root / relative).is_file():
            errors.append(f"missing rc hardening source/doc: {relative}")

    policy = (source_root / "src/features/FeaturePolicy.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    if "ReleaseChannel::Rc" not in policy:
        errors.append("FeaturePolicy must define ReleaseChannel::Rc")

    defaults = (source_root / "src/storage/DefaultSettings.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    if "enablePortableUpdaterPoC" not in defaults:
        errors.append("DefaultSettings must define enablePortableUpdaterPoC")

    return errors


def verify_stable_release_docs(staging: Path) -> list[str]:
    errors: list[str] = []
    for name in STABLE_RELEASE_DOC_FILES:
        if not (staging / "docs" / "release" / name).is_file():
            errors.append(f"missing stable release doc: docs/release/{name}")
    known_issues = staging / "docs" / "release" / "known-issues.md"
    if known_issues.is_file():
        text = known_issues.read_text(encoding="utf-8", errors="replace").lower()
        if "msi" not in text or "portable" not in text:
            errors.append("known-issues.md must mention MSI PoC and portable artifact")
    release_notes = staging / "docs" / "release-notes" / "1.0.0.md"
    if not release_notes.is_file():
        errors.append("missing release notes: docs/release-notes/1.0.0.md")
    return errors


def verify_stable_release_source(source_root: Path) -> list[str]:
    errors: list[str] = []
    required = (
        "src/features/FeaturePolicy.cpp",
        "src/storage/DefaultSettings.cpp",
        "scripts/audit-redaction.py",
        "docs/release/release-scope.md",
        "docs/release/blockers.md",
    )
    for relative in required:
        if not (source_root / relative).is_file():
            errors.append(f"missing stable release source/doc: {relative}")
    return errors


def verify_stable_channel_defaults(source_root: Path) -> list[str]:
    errors: list[str] = []
    defaults = (source_root / "src/storage/DefaultSettings.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    if 'channel == QStringLiteral("stable")' not in defaults:
        errors.append(
            "stable channel must disable experimental features by default in DefaultSettings"
        )
    if "enablePortableUpdaterPoC" in defaults and "return false" not in defaults:
        errors.append("enablePortableUpdaterPoC must default to false for stable")
    return errors


def verify_rc_channel_defaults(source_root: Path) -> list[str]:
    errors: list[str] = []
    defaults = (source_root / "src/storage/DefaultSettings.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    if 'channel == QStringLiteral("rc")' not in defaults:
        errors.append("rc channel must disable experimental features by default in DefaultSettings")
    if "enablePortableUpdaterPoC" in defaults and "return false" not in defaults:
        errors.append("enablePortableUpdaterPoC must default to false for rc/stable")
    return errors


def verify_public_beta_artifact(staging: Path, content: Path | None = None) -> list[str]:
    errors: list[str] = []
    root = content if content is not None else artifact_content_root(staging)
    if not (root / "RELEASE_NOTES.md").is_file():
        errors.append("RELEASE_NOTES.md is missing from artifact")
    if not list(staging.rglob("zarya_en.qm")) or not list(staging.rglob("zarya_ru.qm")):
        errors.append("translations (zarya_en.qm, zarya_ru.qm) are missing from artifact")
    helper_names = ("zarya-helper.exe", "zarya-helper")
    updater_names = ("zarya-updater.exe", "zarya-updater")
    if not any(staging.rglob(name) for name in helper_names):
        errors.append("zarya-helper binary is missing from artifact")
    if not any(staging.rglob(name) for name in updater_names):
        errors.append("zarya-updater binary is missing from artifact")
    return errors


def verify_forbidden_files(staging: Path) -> list[str]:
    errors: list[str] = []
    for path in staging.rglob("*"):
        if not path.is_file():
            continue
        if path.name in FORBIDDEN_ARTIFACT_NAMES:
            errors.append(f"forbidden file included: {path.relative_to(staging)}")
        if path.suffix in {".download", ".tmp"}:
            errors.append(f"forbidden temp file included: {path.relative_to(staging)}")
    return errors


def verify_release_notes_version(staging: Path, expected_version: str, content: Path | None = None) -> list[str]:
    root = content if content is not None else artifact_content_root(staging)
    notes = root / "RELEASE_NOTES.md"
    if not notes.is_file():
        return ["RELEASE_NOTES.md is missing from artifact"]
    if expected_version not in notes.read_text(encoding="utf-8"):
        return [f"RELEASE_NOTES.md does not mention {expected_version}"]
    return []


def verify_no_stale_versions(staging: Path, expected_version: str, content: Path | None = None) -> list[str]:
    errors: list[str] = []
    root = content if content is not None else artifact_content_root(staging)
    parts = expected_version.split(".")
    stale_versions: list[str] = []
    if expected_version == "1.0.0":
        stale_versions.extend(("0.36.0-rc1", "0.35.0-beta"))
    elif len(parts) >= 2 and parts[1].isdigit():
        previous_minor = int(parts[1]) - 1
        if previous_minor >= 0:
            stale_versions.append(f"0.{previous_minor}.0-beta")
            stale_versions.append(f"0.{previous_minor}.0-rc1")

    scan_paths = [root / "RELEASE_NOTES.md"]
    public_beta = root / "docs" / "public-beta"
    if public_beta.is_dir():
        scan_paths.extend(sorted(public_beta.glob("*.md")))

    for path in scan_paths:
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for stale in stale_versions:
            if stale in text:
                rel = path.relative_to(root)
                errors.append(f"stale version {stale} found in {rel}")
    return errors


def verify_executable_version(staging: Path, expected_version: str) -> list[str]:
    candidates = artifact_gui_candidates(staging)
    exe = candidates[0] if candidates else None
    if exe is None:
        return []
    ok, output = run_version_check(exe, gui=True)
    if not ok:
        return [f"executable --version failed for {exe.name}: {output}"]
    if expected_version not in output:
        return [f"executable --version does not report {expected_version}"]
    return []


def detect_platform(artifact: Path, manifest: dict | None) -> str:
    if manifest and manifest.get("platform"):
        return str(manifest["platform"])
    name = artifact.name.lower()
    if "windows" in name:
        return "windows"
    if "macos" in name:
        return "macos"
    if "linux" in name:
        return "linux"
    return "unknown"


def _find_signtool() -> str | None:
    found = shutil.which("signtool")
    if found:
        return found
    candidates = glob.glob(r"C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe")
    return sorted(candidates)[-1] if candidates else None


def verify_windows_signatures(staging: Path) -> tuple[bool, dict]:
    exes = list(staging.rglob("Zarya.exe")) + list(staging.rglob("zarya-helper.exe"))
    if not exes:
        return False, {"windowsAuthenticode": "no executables found"}
    signtool = _find_signtool()
    if not signtool:
        return False, {"windowsAuthenticode": "signtool not available"}
    results: dict[str, str] = {}
    signed_any = False
    for exe in exes:
        proc = subprocess.run(
            [signtool, "verify", "/pa", "/v", str(exe)],
            capture_output=True,
            text=True,
        )
        ok = proc.returncode == 0
        results[exe.name] = "valid" if ok else "unsigned or invalid"
        signed_any = signed_any or ok
    return signed_any, {"windowsAuthenticode": results}


def verify_macos_signatures(app_path: Path) -> tuple[bool, dict]:
    verification: dict[str, str | None] = {
        "macosCodesign": None,
        "macosNotarization": None,
    }
    proc = subprocess.run(
        ["codesign", "--verify", "--deep", "--strict", "--verbose=2", str(app_path)],
        capture_output=True,
        text=True,
    )
    codesign_ok = proc.returncode == 0
    verification["macosCodesign"] = "valid" if codesign_ok else "unsigned or invalid"
    stapler = subprocess.run(
        ["stapler", "validate", str(app_path)],
        capture_output=True,
        text=True,
    )
    verification["macosNotarization"] = "stapled" if stapler.returncode == 0 else "not stapled"
    return codesign_ok, verification


def verify_linux_signatures(artifact: Path) -> tuple[bool, dict]:
    verification: dict[str, str | None] = {"linuxGpg": None, "linuxMinisign": None}
    gpg_sig = Path(f"{artifact}.asc")
    minisig = Path(f"{artifact}.minisig")
    signed = False
    if gpg_sig.is_file():
        proc = subprocess.run(
            ["gpg", "--verify", str(gpg_sig), str(artifact)],
            capture_output=True,
            text=True,
        )
        verification["linuxGpg"] = "valid" if proc.returncode == 0 else "invalid"
        signed = signed or proc.returncode == 0
    if minisig.is_file():
        proc = subprocess.run(
            ["minisign", "-Vm", str(artifact), "-x", str(minisig)],
            capture_output=True,
            text=True,
        )
        verification["linuxMinisign"] = "valid" if proc.returncode == 0 else "invalid"
        signed = signed or proc.returncode == 0
    return signed, verification


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify a Zarya release artifact")
    parser.add_argument("--artifact", required=True, help="Path to .zip or .tar.gz")
    parser.add_argument("--expected-version", help="Expected version string")
    parser.add_argument("--require-checksum", action="store_true", help="Require matching SHA256")
    parser.add_argument("--allow-unsigned", action="store_true", help="Allow unsigned artifacts")
    parser.add_argument("--require-signed", action="store_true", help="Fail if artifact is unsigned")
    parser.add_argument(
        "--public-beta",
        action="store_true",
        help="Verify public beta docs, release notes, translations, helper, and issue templates",
    )
    parser.add_argument(
        "--release-candidate",
        action="store_true",
        help="Verify RC docs, channel defaults, release notes, and stable-scope gating",
    )
    parser.add_argument(
        "--stable-release",
        "--release-stable",
        action="store_true",
        dest="stable_release",
        help="Verify stable release docs, channel defaults, release notes, and stable-scope gating",
    )
    args = parser.parse_args()

    if args.require_signed and args.allow_unsigned:
        print("Cannot combine --require-signed and --allow-unsigned", file=sys.stderr)
        return 2

    artifact = Path(args.artifact)
    if not artifact.is_file():
        print(f"Verification failed: artifact not found: {artifact}", file=sys.stderr)
        return 1

    expected_version = args.expected_version or read_cmake_version()["version"]
    output_dir = artifact.parent
    errors: list[str] = []
    warnings: list[str] = []

    digest = sha256_file(artifact)
    recorded = checksum_for_artifact(output_dir, artifact)
    if args.require_checksum or recorded:
        if not recorded:
            errors.append("SHA256 sidecar or SHA256SUMS.txt entry is missing")
        elif recorded.lower() != digest.lower():
            errors.append("SHA256 checksum mismatch")

    temp_dir = Path(tempfile.mkdtemp(prefix="zarya-verify-"))
    try:
        if artifact.suffix == ".zip":
            extract_zip(artifact, temp_dir)
        elif artifact.name.endswith(".tar.gz"):
            extract_tar_gz(artifact, temp_dir)
        else:
            print(f"Verification failed: unsupported artifact type: {artifact.name}", file=sys.stderr)
            return 1

        staging = find_staging_root(temp_dir)
        content = artifact_content_root(staging)
        errors.extend(verify_forbidden_files(staging))
        if args.public_beta or args.release_candidate or args.stable_release:
            errors.extend(verify_public_beta_docs(content))
            errors.extend(verify_installer_docs(content))
            errors.extend(verify_updater_docs(content))
            errors.extend(verify_stable_docs(content))
            errors.extend(verify_public_beta_artifact(staging, content))
            errors.extend(verify_issue_templates(ROOT))
            errors.extend(verify_installer_skeletons(ROOT))
            errors.extend(verify_updater_source(ROOT))
            errors.extend(verify_stable_source(ROOT))
            errors.extend(verify_no_production_installer_claims(content))
            errors.extend(verify_no_production_self_update_claims(content))
            errors.extend(verify_release_notes_version(staging, expected_version, content))
            errors.extend(verify_no_stale_versions(staging, expected_version, content))
            errors.extend(verify_executable_version(staging, expected_version))
        if args.release_candidate:
            errors.extend(verify_rc_docs(content))
            errors.extend(verify_rc_source(ROOT))
            errors.extend(verify_rc_channel_defaults(ROOT))
            if not expected_version.endswith("-rc1") and "-rc" not in expected_version:
                errors.append(f"expected RC version, got {expected_version}")
        if args.stable_release:
            errors.extend(verify_stable_release_docs(content))
            errors.extend(verify_stable_release_source(ROOT))
            errors.extend(verify_stable_channel_defaults(ROOT))
            if expected_version != "1.0.0":
                errors.append(f"expected stable version 1.0.0, got {expected_version}")
        manifest = load_manifest(staging)
        if manifest is None:
            errors.append("release-manifest.json is missing")
        else:
            if manifest.get("version") != expected_version:
                errors.append(
                    f"version mismatch: expected {expected_version}, found {manifest.get('version')}"
                )
            if args.release_candidate and manifest.get("channel") != "rc":
                errors.append(
                    f"channel mismatch: expected rc, found {manifest.get('channel')}"
                )
            if args.stable_release and manifest.get("channel") != "stable":
                errors.append(
                    f"channel mismatch: expected stable, found {manifest.get('channel')}"
                )
            signing = manifest.get("signing") or {}
            platform = detect_platform(artifact, manifest)
            signed = bool(signing.get("signed"))

            if args.require_signed and not signed:
                errors.append("Verification failed: artifact is not signed")

            if signed or args.require_signed:
                if platform == "windows":
                    ok, details = verify_windows_signatures(staging)
                    if not ok and args.require_signed:
                        errors.append(f"Windows Authenticode verification failed: {details}")
                elif platform == "macos":
                    app = next(staging.rglob("*.app"), None)
                    if app is None:
                        errors.append("macOS .app bundle not found in artifact")
                    else:
                        ok, details = verify_macos_signatures(app)
                        if not ok and args.require_signed:
                            errors.append(f"macOS codesign verification failed: {details}")
                elif platform == "linux":
                    ok, details = verify_linux_signatures(artifact)
                    if not ok and args.require_signed:
                        errors.append(f"Linux signature verification failed: {details}")

            if not signed and (args.allow_unsigned or not args.require_signed):
                warnings.append(
                    "Artifact is unsigned; unsigned artifacts are allowed in this mode."
                )
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)

    if errors:
        for err in errors:
            print(err, file=sys.stderr)
        print("Verification failed", file=sys.stderr)
        return 1

    print("Verification OK")
    for note in warnings:
        print(note)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
