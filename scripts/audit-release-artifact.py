#!/usr/bin/env python3
"""Fail when a release archive contains user data or likely runtime secrets."""

from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import FORBIDDEN_ARTIFACT_NAMES, extract_tar_gz, extract_zip  # noqa: E402

SECRET_FILE_NAMES = {
    *(name.lower() for name in FORBIDDEN_ARTIFACT_NAMES),
    ".env",
    ".env.local",
    "credentials.json",
    "id_ed25519",
    "id_rsa",
}
SECRET_FILE_SUFFIXES = {".key", ".p12", ".pfx", ".kdbx"}
TEXT_FILE_SUFFIXES = {
    "",
    ".cfg",
    ".conf",
    ".ini",
    ".json",
    ".log",
    ".toml",
    ".xml",
    ".yaml",
    ".yml",
}
MAX_TEXT_SCAN_BYTES = 2 * 1024 * 1024
SECRET_PATTERNS = (
    ("private key", re.compile(rb"-----BEGIN [A-Z ]*PRIVATE KEY-----")),
    (
        "helper token",
        re.compile(rb"helper\.token\s*[:=]\s*[\"']?(?!<redacted>)[^\s\"']+", re.IGNORECASE),
    ),
    (
        "credential-bearing URL",
        re.compile(rb"https?://[^\s/:@]+:[^\s/@]+@", re.IGNORECASE),
    ),
    (
        "raw proxy share link",
        re.compile(
            rb"(?:vless|trojan|hysteria2|hy2|wireguard|wg)://[^\s/#]+@"
            rb"|(?:vmess|ss)://[A-Za-z0-9_-]{32,}",
            re.IGNORECASE,
        ),
    ),
)


def audit_tree(root: Path) -> list[str]:
    errors: list[str] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root)
        lower_name = path.name.lower()
        if lower_name in SECRET_FILE_NAMES:
            errors.append(f"forbidden user-data file: {relative}")
            continue
        if path.suffix.lower() in SECRET_FILE_SUFFIXES:
            errors.append(f"private credential file: {relative}")
            continue
        if path.suffix.lower() not in TEXT_FILE_SUFFIXES:
            continue
        try:
            if path.stat().st_size > MAX_TEXT_SCAN_BYTES:
                continue
            content = path.read_bytes()
        except OSError as exc:
            errors.append(f"could not audit {relative}: {exc}")
            continue
        if b"\0" in content:
            continue
        for label, pattern in SECRET_PATTERNS:
            if pattern.search(content):
                errors.append(f"{label} found in {relative}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", required=True, help="Release .zip or .tar.gz archive")
    args = parser.parse_args()

    artifact = Path(args.artifact)
    if not artifact.is_file():
        print(f"Artifact secret audit failed: artifact not found: {artifact}", file=sys.stderr)
        return 1

    temp_dir = Path(tempfile.mkdtemp(prefix="zarya-artifact-audit-"))
    try:
        if artifact.suffix.lower() == ".zip":
            extract_zip(artifact, temp_dir)
        elif artifact.name.lower().endswith(".tar.gz"):
            extract_tar_gz(artifact, temp_dir)
        else:
            print(f"Artifact secret audit failed: unsupported archive: {artifact.name}",
                  file=sys.stderr)
            return 1

        errors = audit_tree(temp_dir)
        if errors:
            print("Artifact secret audit failed:", file=sys.stderr)
            for error in errors:
                print(f"  - {error}", file=sys.stderr)
            return 1
        print(f"Artifact secret audit passed for {artifact.name}")
        return 0
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
