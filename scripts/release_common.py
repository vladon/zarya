#!/usr/bin/env python3
"""Shared helpers for Zarya release packaging and smoke tests."""

from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
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


def download_url(url: str, destination: Path) -> None:
    """Download a release asset atomically (used for pinned geo data only)."""
    destination.parent.mkdir(parents=True, exist_ok=True)
    partial = destination.with_suffix(destination.suffix + ".partial")
    try:
        with urllib.request.urlopen(url, timeout=120) as response, partial.open("wb") as handle:
            shutil.copyfileobj(response, handle)
        partial.replace(destination)
    except (urllib.error.URLError, OSError) as exc:
        partial.unlink(missing_ok=True)
        raise RuntimeError(f"Failed to download {url}: {exc}") from exc

def copy_top_level_legal_files(staging: Path) -> None:
    for name in ("README.md", "LICENSE", "LICENSE.MIT", "LICENSE.GPL-3.0", "COPYING", "THIRD_PARTY_NOTICES.md", "RELEASE_NOTES.md"):
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
    "windows-msi-poc.md",
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
    "portable-update-implementation.md",
    "recovery.md",
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

RC_DOC_FILES = (
    "rc-scope.md",
    "rc-go-no-go.md",
    "rc-regression-matrix.md",
    "rc-known-issues.md",
    "rc-blockers.md",
    "rc-release-process.md",
    "recovery-audit.md",
)

STABLE_RELEASE_DOC_FILES = (
    "README.md",
    "1.5.0-checklist.md",
    "release-scope.md",
    "known-issues.md",
    "blockers.md",
    "go-no-go.md",
    "regression-matrix.md",
    "release-process.md",
    "recovery-audit.md",
)


def copy_stable_docs(staging: Path) -> None:
    dest = staging / "docs" / "stable"
    dest.mkdir(parents=True, exist_ok=True)
    for name in STABLE_DOC_FILES:
        src = ROOT / "docs" / "stable" / name
        if src.is_file():
            shutil.copy2(src, dest / name)


def copy_rc_docs(staging: Path) -> None:
    dest = staging / "docs" / "rc"
    dest.mkdir(parents=True, exist_ok=True)
    for name in RC_DOC_FILES:
        src = ROOT / "docs" / "rc" / name
        if src.is_file():
            shutil.copy2(src, dest / name)
    release_notes = ROOT / "docs" / "release-notes" / "0.36-rc1.md"
    if release_notes.is_file():
        dest_notes = staging / "docs" / "release-notes"
        dest_notes.mkdir(parents=True, exist_ok=True)
        shutil.copy2(release_notes, dest_notes / release_notes.name)


def copy_stable_release_docs(staging: Path) -> None:
    dest = staging / "docs" / "release"
    dest.mkdir(parents=True, exist_ok=True)
    for name in STABLE_RELEASE_DOC_FILES:
        src = ROOT / "docs" / "release" / name
        if src.is_file():
            shutil.copy2(src, dest / name)
    release_notes = ROOT / "docs" / "release-notes" / f"{read_cmake_version()['version']}.md"
    if not release_notes.is_file():
        # Fall back to latest stable notes if a patch-specific file is missing.
        release_notes = ROOT / "docs" / "release-notes" / "1.5.0.md"
    if release_notes.is_file():
        dest_notes = staging / "docs" / "release-notes"
        dest_notes.mkdir(parents=True, exist_ok=True)
        shutil.copy2(release_notes, dest_notes / release_notes.name)


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


def bundle_geodata_enabled(explicit: bool | None = None) -> bool:
    if explicit is not None:
        return explicit
    value = os.environ.get("ZARYA_BUNDLE_GEODATA", "1").strip().lower()
    return value not in {"0", "false", "no", "off"}


def geodata_pin_path() -> Path:
    return ROOT / "packaging" / "geodata" / "runetfreedom-pin.json"


def load_geodata_pin() -> dict[str, Any]:
    path = geodata_pin_path()
    if not path.is_file():
        raise FileNotFoundError(f"Missing geo data pin file: {path}")
    data = json.loads(path.read_text(encoding="utf-8"))
    for key in ("repo", "tag", "version", "sourceId", "assets"):
        if key not in data:
            raise ValueError(f"Invalid runetfreedom-pin.json: missing {key}")
    assets = data["assets"]
    if not isinstance(assets, dict):
        raise ValueError("Invalid runetfreedom-pin.json: assets must be an object")
    for name in ("geoip.dat", "geosite.dat"):
        if name not in assets or "sha256" not in assets[name]:
            raise ValueError(f"Invalid runetfreedom-pin.json: missing assets.{name}.sha256")
    return data


def geodata_bundle_cache_dir() -> Path:
    override = os.environ.get("ZARYA_GEODATA_BUNDLE_CACHE", "").strip()
    if override:
        path = Path(override)
    else:
        path = Path.home() / ".cache" / "zarya-release" / "geodata"
    path.mkdir(parents=True, exist_ok=True)
    return path


def detect_bundled_geodata(install_dir: Path) -> dict[str, Any] | None:
    if not install_dir.is_dir():
        return None
    geoip = install_dir / "geoip.dat"
    geosite = install_dir / "geosite.dat"
    if not geoip.is_file() or not geosite.is_file():
        return None
    version = ""
    source_id = "runetfreedom"
    metadata_path = install_dir / ".zarya-geodata.json"
    if metadata_path.is_file():
        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
            version = str(metadata.get("version") or "").strip()
            source_id = str(metadata.get("sourceId") or source_id).strip() or source_id
        except json.JSONDecodeError:
            pass
    return {
        "version": version,
        "sourceId": source_id,
        "source": "bundled",
        "geoip": geoip,
        "geosite": geosite,
    }


def ensure_bundled_geodata(
    cores_parent: Path,
    *,
    enabled: bool | None = None,
) -> dict[str, Any] | None:
    """Download pinned runetfreedom geo files into cores_parent/cores/xray/."""
    if not bundle_geodata_enabled(enabled):
        return None

    pin = load_geodata_pin()
    tag = str(pin["tag"]).strip()
    version = str(pin.get("version") or tag).strip()
    repo = str(pin["repo"]).strip()
    source_id = str(pin.get("sourceId") or "runetfreedom").strip()
    install_dir = cores_parent / "cores" / "xray"
    install_dir.mkdir(parents=True, exist_ok=True)
    cache_dir = geodata_bundle_cache_dir() / tag
    cache_dir.mkdir(parents=True, exist_ok=True)

    for file_name in ("geoip.dat", "geosite.dat"):
        expected_sha = str(pin["assets"][file_name]["sha256"]).strip().lower()
        if expected_sha.startswith("sha256:"):
            expected_sha = expected_sha.split(":", 1)[1]
        if len(expected_sha) != 64:
            raise ValueError(f"Invalid sha256 for {file_name} in runetfreedom-pin.json")
        url = f"https://github.com/{repo}/releases/download/{tag}/{file_name}"
        cache_path = cache_dir / file_name
        if cache_path.is_file() and sha256_file(cache_path).lower() != expected_sha:
            cache_path.unlink()
        if not cache_path.is_file():
            print(f"Downloading bundled geo data {tag} ({file_name})...")
            download_url(url, cache_path)
        actual_sha = sha256_file(cache_path).lower()
        if actual_sha != expected_sha:
            raise RuntimeError(
                f"Checksum mismatch for {file_name}: expected {expected_sha}, got {actual_sha}"
            )
        destination = install_dir / file_name
        if destination.exists():
            destination.unlink()
        shutil.copy2(cache_path, destination)

    metadata = {
        "version": version,
        "sourceId": source_id,
        "installedAt": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "source": "bundled",
        "bundledTag": tag,
        "repo": repo,
    }
    (install_dir / ".zarya-geodata.json").write_text(
        json.dumps(metadata, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Bundled geo data {source_id} {version} -> {install_dir}")
    return detect_bundled_geodata(install_dir)


def verify_bundled_geodata_manifest(
    staging: Path, manifest: dict[str, Any]
) -> list[str]:
    errors: list[str] = []
    included = manifest.get("included") or {}
    install_candidates: list[Path] = [
        staging / "cores" / "xray",
        staging / "Contents" / "MacOS" / "cores" / "xray",
    ]
    if staging.name == "Resources" and staging.parent.name == "Contents":
        install_candidates.append(staging.parent / "MacOS" / "cores" / "xray")
    app_bundle = next(staging.rglob("*.app"), None)
    if app_bundle is None and staging.name.endswith(".app"):
        app_bundle = staging
    if app_bundle is not None:
        install_candidates.append(app_bundle / "Contents" / "MacOS" / "cores" / "xray")
    for match in staging.rglob("cores/xray"):
        if match.is_dir():
            install_candidates.append(match)

    detected = None
    seen: set[str] = set()
    for candidate in install_candidates:
        key = str(candidate.resolve()) if candidate.exists() else str(candidate)
        if key in seen:
            continue
        seen.add(key)
        detected = detect_bundled_geodata(candidate)
        if detected:
            break

    if included.get("geoData"):
        if detected is None:
            errors.append("release-manifest.json marks geoData bundled, but geoip/geosite are missing")
        else:
            expected_version = str(included.get("geoDataVersion") or "").strip()
            if expected_version and detected.get("version") != expected_version:
                errors.append(
                    "bundled geo data version mismatch: "
                    f"manifest={expected_version}, on-disk={detected.get('version')}"
                )
            expected_source = str(included.get("geoDataSource") or "").strip()
            if expected_source and detected.get("sourceId") != expected_source:
                errors.append(
                    "bundled geo data source mismatch: "
                    f"manifest={expected_source}, on-disk={detected.get('sourceId')}"
                )
    elif detected is not None and detected.get("source") == "bundled":
        errors.append("bundled geo data present but included.geoData is false")
    return errors


def verify_bundled_xray_manifest(
    staging: Path, manifest: dict[str, Any], *, platform: str | None = None
) -> list[str]:
    errors: list[str] = []
    included = manifest.get("included") or {}
    plat = (platform or str(manifest.get("platform") or "")).strip().lower()
    if included.get("xrayDistribution") != "embedded":
        errors.append("release-manifest.json must declare embedded Xray")
    if not included.get("xray"):
        errors.append("release-manifest.json must include embedded Xray")
    if not str(included.get("xrayCommit") or "").strip():
        errors.append("release-manifest.json is missing embedded Xray source commit")
    for candidate in staging.rglob("*"):
        if candidate.is_file() and candidate.name.lower() in {"xray", "xray.exe"}:
            errors.append(f"embedded Xray package must not contain an Xray executable: {candidate}")
            break
    bridge = str((manifest.get("artifacts") or {}).get("xrayBridge") or "").strip()
    if plat not in {"macos", "darwin"}:
        if not bridge:
            errors.append("embedded Xray bridge artifact is not declared")
        elif not (staging / bridge).is_file():
            errors.append(f"embedded Xray bridge is missing: {bridge}")
    return errors

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
        "note": (
            "This stable build may be unsigned. Use SHA256 checksums from the release page."
            if read_cmake_version()["channel"] == "stable"
            else "This release-candidate build may be unsigned. Use SHA256 checksums from the release page."
            if read_cmake_version()["channel"] == "rc"
            else "This beta build is unsigned. Use SHA256 checksums from the release page."
        ),
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
    updater_artifact: str | None = None,
    version: str | None = None,
    channel: str | None = None,
    build_commit: str | None = None,
    signing: dict[str, Any] | None = None,
    artifact_type: str | None = None,
    installation_mode: str | None = None,
    helper_service: dict[str, Any] | None = None,
    bundle_geodata: bool | None = None,
    embedded_xray_artifact: str | None = None,
    embedded_singbox_artifact: str | None = None,
    core_test_worker_artifact: str | None = None,
    cores_parent: Path | None = None,
) -> Path:
    meta = read_cmake_version()
    version = version or meta["version"]
    channel = channel or meta["channel"]
    build_commit = build_commit or git_commit_short()

    asset_parent = cores_parent or staging
    bundled_geo = ensure_bundled_geodata(asset_parent, enabled=bundle_geodata)
    if bundled_geo is None:
        bundled_geo = detect_bundled_geodata(asset_parent / "cores" / "xray")

    checksum_names = [gui_artifact]
    if helper_artifact:
        checksum_names.append(helper_artifact)
    if updater_artifact:
        checksum_names.append(updater_artifact)
    if core_test_worker_artifact:
        checksum_names.append(core_test_worker_artifact)

    included: dict[str, Any] = {
        "translations": ["en", "ru"],
        "xray": False,
        "singBox": False,
        "geoData": False,
        "ruleSets": False,
    }
    source = json.loads((ROOT / "third_party" / "xray-core.zarya.json").read_text(encoding="utf-8"))
    included["xray"] = True
    included["xrayVersion"] = source.get("tag") or ""
    included["xraySource"] = "vendored"
    included["xrayDistribution"] = "embedded"
    included["xrayCommit"] = source.get("commit") or ""
    included["xrayAbiVersion"] = 1
    singbox_source = json.loads((ROOT / "third_party" / "sing-box.zarya.json").read_text(encoding="utf-8"))
    included["singBox"] = True
    included["singBoxVersion"] = singbox_source.get("tag") or ""
    included["singBoxSource"] = "vendored"
    included["singBoxDistribution"] = "embedded-helper"
    included["singBoxCommit"] = singbox_source.get("commit") or ""
    included["singBoxAbiVersion"] = 1
    if bundled_geo is not None:
        included["geoData"] = True
        included["geoDataSource"] = bundled_geo.get("sourceId") or "runetfreedom"
        included["geoDataVersion"] = bundled_geo.get("version") or ""

    checksums = file_sha256_map(staging, checksum_names)
    if embedded_xray_artifact:
        bridge_path = staging / embedded_xray_artifact
        if not bridge_path.is_file():
            raise FileNotFoundError(f"embedded Xray artifact is missing: {bridge_path}")
        checksums[embedded_xray_artifact] = f"sha256:{sha256_file(bridge_path)}"

    if embedded_singbox_artifact:
        bridge_path = staging / embedded_singbox_artifact
        if not bridge_path.is_file():
            raise FileNotFoundError(f"embedded sing-box artifact is missing: {bridge_path}")
        checksums[embedded_singbox_artifact] = f"sha256:{sha256_file(bridge_path)}"

    if bundled_geo is not None:
        for key in ("geoip", "geosite"):
            path = Path(bundled_geo[key])
            try:
                rel = path.relative_to(staging).as_posix()
            except ValueError:
                rel = f"cores/xray/{path.name}"
            checksums[rel] = f"sha256:{sha256_file(path)}"

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
        "included": included,
        "features": {
            "systemProxyXray": True,
            "tunSingBoxExperimental": True,
            "linuxNftKillSwitchPoC": True,
            "windowsWfpKillSwitchPoC": True,
            "macosKillSwitch": False,
        },
        "checksums": checksums,
    }
    if helper_artifact:
        manifest["artifacts"]["helper"] = helper_artifact
    if updater_artifact:
        manifest["artifacts"]["updater"] = updater_artifact
    if embedded_xray_artifact:
        manifest["artifacts"]["xrayBridge"] = embedded_xray_artifact
    if embedded_singbox_artifact:
        manifest["artifacts"]["singBoxBridge"] = embedded_singbox_artifact
    if core_test_worker_artifact:
        manifest["artifacts"]["coreTestWorker"] = core_test_worker_artifact

    manifest["signing"] = signing if signing is not None else default_unsigned_signing()

    if artifact_type:
        manifest["artifactType"] = artifact_type
    if installation_mode:
        manifest["installationMode"] = installation_mode
    if helper_service is not None:
        manifest["helperService"] = helper_service

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


def refresh_manifest_checksums(staging: Path) -> Path | None:
    """Refresh checksums after an external signer has modified packaged binaries."""
    manifest_path = find_release_manifest(staging)
    if manifest_path is None:
        return None
    data = json.loads(manifest_path.read_text(encoding="utf-8"))
    checksums = data.get("checksums")
    if not isinstance(checksums, dict):
        return manifest_path
    for relative in list(checksums):
        path = staging / relative
        if path.is_file():
            checksums[relative] = f"sha256:{sha256_file(path)}"
    manifest_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return manifest_path


def verify_clean_staging(staging: Path) -> list[str]:
    errors: list[str] = []
    content = artifact_content_root(staging)
    if not (content / "LICENSE").is_file():
        errors.append("LICENSE is missing from staging")
    for path in staging.rglob("*"):
        if path.is_file() and path.name in FORBIDDEN_ARTIFACT_NAMES:
            errors.append(f"forbidden file included: {path.relative_to(staging)}")
    return errors


def verify_required_paths(staging: Path, required: list[str]) -> list[str]:
    missing = [item for item in required if not (staging / item).exists()]
    return [f"missing required path: {item}" for item in missing]


def artifact_content_root(staging: Path) -> Path:
    if staging.name.endswith(".app"):
        resources = staging / "Contents" / "Resources"
        if resources.is_dir():
            return resources
    return staging


def artifact_gui_candidates(staging: Path) -> list[Path]:
    names = ("Zarya.exe", "zarya", "Zarya")
    if staging.name.endswith(".app"):
        macos = staging / "Contents" / "MacOS"
        if macos.is_dir():
            return [macos / name for name in names if (macos / name).is_file()]
    return [path for name in names for path in staging.rglob(name) if path.is_file()]


def ensure_executable(path: Path) -> None:
    if path.is_file():
        path.chmod(path.stat().st_mode | 0o111)


def run_version_check(executable: Path, *, gui: bool = False) -> tuple[bool, str]:
    if not executable.is_file():
        return False, f"executable not found: {executable}"
    if gui and os.environ.get("ZARYA_SKIP_GUI_VERSION") == "1":
        return True, "skipped (ZARYA_SKIP_GUI_VERSION=1)"
    ensure_executable(executable)
    cmd = [str(executable), "--version"]
    if gui and sys.platform == "linux" and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if xvfb:
            cmd = [xvfb, "-a", *cmd]
    try:
        result = subprocess.run(
            cmd,
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
