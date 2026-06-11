#!/usr/bin/env python3
"""Audit diagnostics bundles and redacted backups for leaked secrets."""

from __future__ import annotations

import argparse
import re
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import extract_zip  # noqa: E402

FORBIDDEN_PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    ("vless:// link", re.compile(r"vless://", re.IGNORECASE)),
    ("vmess:// link", re.compile(r"vmess://", re.IGNORECASE)),
    ("trojan:// link", re.compile(r"trojan://", re.IGNORECASE)),
    ("ss:// link", re.compile(r"ss://", re.IGNORECASE)),
    ("helper.token secret", re.compile(r"helper\.token\s*=\s*[^\s<]+", re.IGNORECASE)),
    (
        "runtime token",
        re.compile(r"runtime[_-]?token\s*[:=]\s*[^\s<\"']+", re.IGNORECASE),
    ),
    (
        "password field",
        re.compile(
            r'"password"\s*:\s*"(?!(\s*<redacted>\s*|<redacted-host>|hasUuid|uuidRedacted))[^"]{4,}"',
            re.IGNORECASE,
        ),
    ),
    (
        "uuid field",
        re.compile(
            r'"uuid"\s*:\s*"(?!(\s*<redacted>\s*|hasUuid|uuidRedacted))'
            r'[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"',
            re.IGNORECASE,
        ),
    ),
    (
        "publicKey field",
        re.compile(
            r'"publicKey"\s*:\s*"(?!(\s*<redacted>\s*))[^"]{8,}"',
            re.IGNORECASE,
        ),
    ),
    (
        "shortId field",
        re.compile(
            r'"shortId"\s*:\s*"(?!(\s*<redacted>\s*))[^"]{2,}"',
            re.IGNORECASE,
        ),
    ),
]

ALLOWED_PLACEHOLDERS = (
    "<redacted>",
    "<redacted-host>",
    "hasUuid",
    "uuidRedacted",
)


def iter_text_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() in {".json", ".txt", ".log", ".md", ".xml", ".ini", ".conf"}:
            files.append(path)
        elif path.name in {"manifest.json", "support-summary.txt"}:
            files.append(path)
    return files


def scan_text(text: str, source: str) -> list[str]:
    errors: list[str] = []
    for label, pattern in FORBIDDEN_PATTERNS:
        for match in pattern.finditer(text):
            snippet = match.group(0)
            if any(placeholder in snippet for placeholder in ALLOWED_PLACEHOLDERS):
                continue
            errors.append(f"{source}: forbidden {label}: {snippet[:80]}")
    return errors


def audit_archive(archive: Path) -> list[str]:
    errors: list[str] = []
    temp_dir = Path(tempfile.mkdtemp(prefix="zarya-redaction-audit-"))
    try:
        if archive.suffix == ".zip":
            extract_zip(archive, temp_dir)
        else:
            errors.append(f"unsupported archive type: {archive.name}")
            return errors

        for path in iter_text_files(temp_dir):
            try:
                content = path.read_text(encoding="utf-8", errors="replace")
            except OSError as exc:
                errors.append(f"{path.name}: cannot read file: {exc}")
                continue
            relative = path.relative_to(temp_dir)
            errors.extend(scan_text(content, str(relative)))
    finally:
        import shutil

        shutil.rmtree(temp_dir, ignore_errors=True)
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit Zarya diagnostics/redacted backups")
    parser.add_argument("--diagnostics", help="Path to diagnostics .zip")
    parser.add_argument("--redacted-backup", help="Path to redacted backup .zip")
    args = parser.parse_args()

    if not args.diagnostics and not args.redacted_backup:
        print("Provide --diagnostics and/or --redacted-backup", file=sys.stderr)
        return 2

    errors: list[str] = []
    if args.diagnostics:
        path = Path(args.diagnostics)
        if not path.is_file():
            print(f"Diagnostics archive not found: {path}", file=sys.stderr)
            return 1
        errors.extend(audit_archive(path))

    if args.redacted_backup:
        path = Path(args.redacted_backup)
        if not path.is_file():
            print(f"Redacted backup archive not found: {path}", file=sys.stderr)
            return 1
        errors.extend(audit_archive(path))

    if errors:
        for err in errors:
            print(err, file=sys.stderr)
        print("Redaction audit failed", file=sys.stderr)
        return 1

    print("Redaction audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
