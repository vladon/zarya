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
    extract_tar_gz,
    extract_zip,
    read_cmake_version,
    sha256_file,
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


def verify_forbidden_files(staging: Path) -> list[str]:
    errors: list[str] = []
    for path in staging.rglob("*"):
        if path.is_file() and path.name in FORBIDDEN_ARTIFACT_NAMES:
            errors.append(f"forbidden file included: {path.relative_to(staging)}")
    return errors


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
        errors.extend(verify_forbidden_files(staging))
        manifest = load_manifest(staging)
        if manifest is None:
            errors.append("release-manifest.json is missing")
        else:
            if manifest.get("version") != expected_version:
                errors.append(
                    f"version mismatch: expected {expected_version}, found {manifest.get('version')}"
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
