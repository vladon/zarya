#!/usr/bin/env python3
"""Shared helpers for Zarya release packaging and smoke tests."""

from __future__ import annotations

import hashlib
import json
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]

DOC_FILES = (
    "known-limitations.md",
    "recovery.md",
    "diagnostics-bundle.md",
    "backup-import-export.md",
    "core-update-manager.md",
    "localization.md",
)

FORBIDDEN_ARTIFACT_NAMES = {
    "helper.token",
    "profiles.json",
    "subscriptions.json",
    "config-xray.json",
    "sing-box-tun.json",
    "killswitch-active.json",
}


def read_cmake_version() -> dict[str, str]:
    text = (ROOT / "cmake" / "ZaryaVersion.cmake").read_text(encoding="utf-8")

    def grab(name: str, default: str = "") -> str:
        match = re.search(rf'set\({name} "([^"]*)"\)', text)
        return match.group(1) if match else default

    return {
        "version": grab("ZARYA_VERSION_STRING", "0.0.0"),
        "channel": grab("ZARYA_BUILD_CHANNEL", "beta"),
    }


def git_commit_short() -> str:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        return result.stdout.strip() or "unknown"
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def copy_top_level_legal_files(staging: Path) -> None:
    for name in ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md", "RELEASE_NOTES.md"):
        src = ROOT / name
        if src.is_file():
            shutil.copy2(src, staging / name)


def copy_docs(staging: Path) -> None:
    docs_dir = staging / "docs"
    docs_dir.mkdir(parents=True, exist_ok=True)
    for name in DOC_FILES:
        src = ROOT / "docs" / name
        if src.is_file():
            shutil.copy2(src, docs_dir / name)
    service_docs = ROOT / "docs" / "service"
    if service_docs.is_dir():
        dest_service = docs_dir / "service"
        dest_service.mkdir(parents=True, exist_ok=True)
        for src in service_docs.glob("*.md"):
            shutil.copy2(src, dest_service / src.name)


PUBLIC_BETA_DOC_FILES = (
    "README.md",
    "quick-start.md",
    "download-verification.md",
    "reporting-issues.md",
    "known-limitations.md",
    "experimental-features.md",
    "privacy-and-diagnostics.md",
    "security.md",
    "beta-checklist.md",
    "feedback-triage.md",
    "beta-blockers.md",
)


INSTALLER_DOC_FILES = (
    "README.md",
    "installed-layout.md",
    "windows-installer-strategy.md",
    "macos-installer-strategy.md",
    "linux-packaging-strategy.md",
    "portable-to-installed-migration.md",
    "uninstall-repair.md",
    "helper-service-installation.md",
    "installer-security.md",
)


def copy_installer_docs(staging: Path) -> None:
    dest = staging / "docs" / "installer"
    dest.mkdir(parents=True, exist_ok=True)
    for name in INSTALLER_DOC_FILES:
        src = ROOT / "docs" / "installer" / name
        if src.is_file():
            shutil.copy2(src, dest / name)


UPDATER_DOC_FILES = (
    "README.md",
    "update-manifest.md",
    "portable-update-flow.md",
    "installed-update-flow.md",
    "updater-security.md",
    "helper-update.md",
)


STABLE_DOC_FILES = (
    "README.md",
    "stable-scope.md",
    "release-criteria.md",
    "risk-register.md",
    "1.0-backlog.md",
    "feature-gating.md",
    "regression-matrix.md",
    "go-no-go-checklist.md",
)


def copy_stable_docs(staging: Path) -> None:
    dest = staging / "docs" / "stable"
    dest.mkdir(parents=True, exist_ok=True)
    for name in STABLE_DOC_FILES:
        src = ROOT / "docs" / "stable" / name
        if src.is_file():
            shutil.copy2(src, dest / name)


def copy_updater_docs(staging: Path) -> None:
    dest = staging / "docs" / "updater"
    dest.mkdir(parents=True, exist_ok=True)
    for name in UPDATER_DOC_FILES:
        src = ROOT / "docs" / "updater" / name
        if src.is_file():
            shutil.copy2(src, dest / name)


def copy_public_beta_docs(staging: Path) -> None:
    dest = staging / "docs" / "public-beta"
    dest.mkdir(parents=True, exist_ok=True)
    for name in PUBLIC_BETA_DOC_FILES:
        src = ROOT / "docs" / "public-beta" / name
        if src.is_file():
            shutil.copy2(src, dest / name)


def copy_service_packaging_templates(staging: Path) -> None:
    packaging_dir = staging / "packaging"
    packaging_dir.mkdir(parents=True, exist_ok=True)
    for relative in (
        "linux/systemd/zarya-helper.service",
        "linux/polkit/dev.vladon.zarya.helper.policy",
        "linux/dbus/dev.vladon.zarya.helper.conf",
        "macos/LaunchDaemon/dev.vladon.zarya.helper.plist",
    ):
        src = ROOT / "packaging" / relative
        if src.is_file():
            dest = packaging_dir / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dest)


def copy_translations(staging: Path, build_translations: Path | None = None) -> None:
    dest = staging / "translations"
    dest.mkdir(parents=True, exist_ok=True)
    sources = []
    if build_translations and build_translations.is_dir():
        sources.extend(build_translations.glob("*.qm"))
    sources.extend((ROOT / "translations").glob("*.qm"))
    copied: set[str] = set()
    for src in sources:
        if src.name in copied:
            continue
        shutil.copy2(src, dest / src.name)
        copied.add(src.name)


def create_placeholder_layout(staging: Path) -> None:
    for relative in ("data", "runtime"):
        path = staging / relative
        path.mkdir(parents=True, exist_ok=True)
        keep = path / ".keep"
        if not keep.exists():
            keep.write_text("", encoding="utf-8")

    for core in ("cores/xray", "cores/sing-box"):
        path = staging / core
        path.mkdir(parents=True, exist_ok=True)
        readme_name = "cores-xray-README.txt" if "xray" in core else "cores-sing-box-README.txt"
        src = ROOT / "packaging" / "windows" / readme_name
        if src.is_file():
            shutil.copy2(src, path / "README.txt")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_checksum_sidecars(output_dir: Path, artifact: Path) -> str:
    digest = sha256_file(artifact)
    sidecar = output_dir / f"{artifact.name}.sha256"
    sidecar.write_text(f"{digest}  {artifact.name}\n", encoding="utf-8")

    sums_path = output_dir / "SHA256SUMS.txt"
    lines: list[str] = []
    if sums_path.is_file():
        lines = [line for line in sums_path.read_text(encoding="utf-8").splitlines() if line]
    lines = [line for line in lines if not line.endswith(f"  {artifact.name}")]
    lines.append(f"{digest}  {artifact.name}")
    sums_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return digest


def default_unsigned_signing() -> dict[str, Any]:
    return {
        "signed": False,
        "signatureType": None,
        "notarized": False,
        "timestamped": False,
        "verification": {
            "windowsAuthenticode": None,
            "macosCodesign": None,
            "macosNotarization": None,
            "linuxGpg": None,
            "linuxMinisign": None,
        },
    }


def build_signing_manifest(
    *,
    signed: bool = False,
    signature_type: str | None = None,
    notarized: bool = False,
    timestamped: bool = False,
    verification: dict[str, Any] | None = None,
) -> dict[str, Any]:
    manifest = default_unsigned_signing()
    manifest["signed"] = signed
    manifest["signatureType"] = signature_type
    manifest["notarized"] = notarized
    manifest["timestamped"] = timestamped
    if verification:
        manifest["verification"].update(verification)
    return manifest


def write_build_integrity(staging: Path, signing: dict[str, Any] | None = None) -> Path:
    payload = {
        "version": read_cmake_version()["version"],
        "signed": False,
        "signatureType": None,
        "notarized": False,
        "timestamped": False,
        "note": "This beta build is unsigned. Use SHA256 checksums from the release page.",
    }
    if signing:
        payload.update(
            {
                "signed": signing.get("signed", False),
                "signatureType": signing.get("signatureType"),
                "notarized": signing.get("notarized", False),
                "timestamped": signing.get("timestamped", False),
            }
        )
        if signing.get("signed"):
            payload["note"] = ""
    path = staging / "build-integrity.json"
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return path


def file_sha256_map(staging: Path, names: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    for name in names:
        path = staging / name
        if path.is_file():
            result[name] = f"sha256:{sha256_file(path)}"
    return result


def write_release_manifest(
    staging: Path,
    *,
    platform: str,
    architecture: str,
    portable: bool,
    gui_artifact: str,
    helper_artifact: str | None,
    version: str | None = None,
    channel: str | None = None,
    build_commit: str | None = None,
    signing: dict[str, Any] | None = None,
) -> Path:
    meta = read_cmake_version()
    version = version or meta["version"]
    channel = channel or meta["channel"]
    build_commit = build_commit or git_commit_short()

    checksum_names = [gui_artifact]
    if helper_artifact:
        checksum_names.append(helper_artifact)

    manifest: dict[str, Any] = {
        "app": "Zarya",
        "version": version,
        "channel": channel,
        "buildCommit": build_commit,
        "buildDateUtc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "platform": platform,
        "architecture": architecture,
        "portable": portable,
        "artifacts": {"gui": gui_artifact},
        "included": {
            "translations": ["en", "ru"],
            "xray": False,
            "singBox": False,
            "geoData": False,
            "ruleSets": False,
        },
        "features": {
            "systemProxyXray": True,
            "tunSingBoxExperimental": True,
            "linuxNftKillSwitchPoC": True,
            "windowsWfpKillSwitchPoC": True,
            "macosKillSwitch": False,
        },
        "checksums": file_sha256_map(staging, checksum_names),
    }
    if helper_artifact:
        manifest["artifacts"]["helper"] = helper_artifact

    manifest["signing"] = signing if signing is not None else default_unsigned_signing()

    path = staging / "release-manifest.json"
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return path


def find_release_manifest(staging: Path) -> Path | None:
    direct = staging / "release-manifest.json"
    if direct.is_file():
        return direct
    matches = sorted(staging.rglob("release-manifest.json"))
    return matches[0] if matches else None


def update_manifest_signing(staging: Path, signing: dict[str, Any]) -> Path | None:
    manifest_path = find_release_manifest(staging)
    if manifest_path is None:
        return None
    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    data["signing"] = signing
    manifest_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return manifest_path


def verify_clean_staging(staging: Path) -> list[str]:
    errors: list[str] = []
    if not (staging / "LICENSE").is_file():
        errors.append("LICENSE is missing from staging")
    for path in staging.rglob("*"):
        if path.is_file() and path.name in FORBIDDEN_ARTIFACT_NAMES:
            errors.append(f"forbidden file included: {path.relative_to(staging)}")
    return errors


def verify_required_paths(staging: Path, required: list[str]) -> list[str]:
    missing = [item for item in required if not (staging / item).exists()]
    return [f"missing required path: {item}" for item in missing]


def run_version_check(executable: Path) -> tuple[bool, str]:
    if not executable.is_file():
        return False, f"executable not found: {executable}"
    try:
        result = subprocess.run(
            [str(executable), "--version"],
            check=True,
            capture_output=True,
            text=True,
            timeout=30,
        )
        return True, result.stdout.strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        return False, str(exc)


def extract_zip(archive: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive, "r") as zf:
        zf.extractall(dest)


def extract_tar_gz(archive: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive, "r:gz") as tf:
        tf.extractall(dest, filter="data")


def main_verify(argv: list[str]) -> int:
    if len(argv) < 3:
        print("usage: release_common.py verify <staging-dir> <req1> [req2 ...]", file=sys.stderr)
        return 2
    staging = Path(argv[1])
    required = argv[2:]
    errors = verify_clean_staging(staging) + verify_required_paths(staging, required)
    if errors:
        for err in errors:
            print(err, file=sys.stderr)
        return 1
    print("staging verification passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main_verify(sys.argv))
