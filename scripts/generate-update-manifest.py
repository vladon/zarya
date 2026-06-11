#!/usr/bin/env python3
"""Generate Zarya update-manifest.json from dist artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import read_cmake_version, sha256_file  # noqa: E402

ARTIFACT_RE = re.compile(
    r"^Zarya-(?P<version>[^-]+(?:-[^-]+)?)-(?P<platform>windows|macos|linux)-"
    r"(?P<arch>x64|arm64|amd64|aarch64)(?:-(?P<mode>portable))?"
    r"\.(?P<ext>zip|tar\.gz)$",
    re.IGNORECASE,
)


def normalize_platform(token: str) -> str:
    return token.lower()


def normalize_arch(token: str) -> str:
    lower = token.lower()
    if lower in {"amd64", "x64", "x86_64"}:
        return "x64"
    if lower in {"arm64", "aarch64"}:
        return "arm64"
    return lower


def detect_installation_mode(name: str, explicit_mode: str | None) -> str:
    if explicit_mode:
        return explicit_mode
    lower = name.lower()
    if "portable" in lower or lower.endswith(".tar.gz"):
        return "portable"
    if any(x in lower for x in (".msi", ".pkg", ".deb", ".rpm", ".appimage")):
        return "installed"
    return "portable"


def build_asset_url(base_url: str | None, file_name: str) -> str:
    if not base_url:
        return file_name
    base = base_url.rstrip("/")
    return f"{base}/{file_name}"


def scan_dist(dist_dir: Path, channel: str, version: str, base_url: str | None) -> list[dict]:
    assets: list[dict] = []
    if not dist_dir.is_dir():
        return assets

    for path in sorted(dist_dir.iterdir()):
        if not path.is_file():
            continue
        if path.suffix not in {".zip", ".gz"} and not path.name.endswith(".tar.gz"):
            continue
        match = ARTIFACT_RE.match(path.name)
        if not match:
            continue
        file_version = match.group("version")
        if file_version != version:
            continue
        platform = normalize_platform(match.group("platform"))
        architecture = normalize_arch(match.group("arch"))
        installation_mode = detect_installation_mode(path.name, match.group("mode"))
        digest = sha256_file(path)
        assets.append(
            {
                "platform": platform,
                "architecture": architecture,
                "installationMode": installation_mode,
                "fileName": path.name,
                "url": build_asset_url(base_url, path.name),
                "sizeBytes": path.stat().st_size,
                "sha256": digest,
                "signature": {"type": "none", "url": None},
            }
        )
    return assets


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Zarya update-manifest.json")
    parser.add_argument("--version", help="Release version (default: from CMake)")
    parser.add_argument("--channel", default="beta", help="Update channel key")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist", help="Artifacts directory")
    parser.add_argument("--base-url", help="Base URL for asset downloads (optional)")
    parser.add_argument(
        "--min-supported-version",
        help="Minimum supported version (default: same major.minor line)",
    )
    parser.add_argument("--output", type=Path, default=Path("update-manifest.json"))
    args = parser.parse_args()

    version_info = read_cmake_version()
    version = args.version or version_info["version"]
    min_supported = args.min_supported_version or "0.30.0-beta"

    assets = scan_dist(args.dist_dir, args.channel, version, args.base_url)
    manifest = {
        "format": "zarya-update-manifest",
        "formatVersion": 1,
        "generatedAt": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "channels": {
            args.channel: {
                "latestVersion": version,
                "minSupportedVersion": min_supported,
                "releaseNotesUrl": "https://example.invalid/releases/"
                + version.replace("/", ""),
                "mandatory": False,
                "assets": assets,
            }
        },
    }

    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output} ({len(assets)} assets)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
