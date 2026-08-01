#!/usr/bin/env python3
"""Translation checks for CI and local development."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TS_RU = ROOT / "translations" / "zarya_ru.ts"
TS_EN = ROOT / "translations" / "zarya_en.ts"
SRC_DIR = ROOT / "src"
QM_DIR = ROOT / "translations"

CYRILLIC_IN_STRING = re.compile(r'QStringLiteral\s*\(\s*"[^"]*[А-Яа-яЁё]')
TR_RUSSIAN = re.compile(r'\btr\s*\(\s*"[^"]*[А-Яа-яЁё]')


def check_ts_exists() -> list[str]:
    errors: list[str] = []
    if not TS_RU.is_file():
        errors.append("translations/zarya_ru.ts is missing")
    if not TS_EN.is_file():
        errors.append("translations/zarya_en.ts is missing")
    return errors


def check_russian_source_strings() -> list[str]:
    errors: list[str] = []
    ui_files = list((SRC_DIR / "ui").rglob("*.cpp"))
    ui_files += list((SRC_DIR / "errors").glob("*.cpp"))
    ui_files += list((SRC_DIR / "i18n").glob("*.cpp"))
    skip_files = {SRC_DIR / "i18n" / "LanguageManager.cpp"}  # native language names
    for path in ui_files:
        if path in skip_files:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern in (CYRILLIC_IN_STRING, TR_RUSSIAN):
            for match in pattern.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                errors.append(f"{path.relative_to(ROOT)}:{line}: Russian in UI source string")
    return errors


def check_ru_coverage() -> tuple[list[str], float]:
    errors: list[str] = []
    if not TS_RU.is_file():
        return ["translations/zarya_ru.ts missing"], 0.0

    root = ET.parse(TS_RU).getroot()
    total = 0
    unfinished = 0
    for msg in root.iter("message"):
        src = msg.find("source")
        if src is None or not "".join(src.itertext()).strip():
            continue
        total += 1
        tr = msg.find("translation")
        if tr is None:
            unfinished += 1
            continue
        if tr.attrib.get("type") == "unfinished":
            unfinished += 1
            continue
        if not list(tr) and not (tr.text and tr.text.strip()):
            unfinished += 1

    coverage = 100.0 * (total - unfinished) / total if total else 0.0
    if coverage < 50.0:
        errors.append(f"Russian coverage too low: {coverage:.0f}% ({unfinished}/{total} unfinished)")
    return errors, coverage


def find_lupdate() -> Path | None:
    names = ("lupdate", "lupdate6", "lupdate-qt6")
    for name in names:
        found = shutil.which(name)
        if found:
            return Path(found)

    prefixes: list[Path] = []
    for variable in ("QT_HOST_PATH", "QT_STATIC_DIR", "QTDIR"):
        value = os.environ.get(variable)
        if value:
            prefixes.append(Path(value))
    for value in os.environ.get("CMAKE_PREFIX_PATH", "").split(os.pathsep):
        if value:
            prefixes.append(Path(value))

    suffixes = (Path("bin/lupdate"), Path("bin/lupdate.exe"))
    for prefix in prefixes:
        for suffix in suffixes:
            candidate = prefix / suffix
            if candidate.is_file():
                return candidate
    return None


def catalog_pairs(path: Path) -> set[tuple[str, str]]:
    pairs: set[tuple[str, str]] = set()
    root = ET.parse(path).getroot()
    for context in root.findall("context"):
        name = (context.findtext("name") or "").removeprefix("zarya::")
        for message in context.findall("message"):
            source = message.findtext("source")
            if source is not None:
                pairs.add((name, source))
    return pairs


def check_source_catalog_coverage(skip_source_scan: bool) -> list[str]:
    if skip_source_scan:
        print("Note: full-source translation scan skipped by request")
        return []

    lupdate = find_lupdate()
    if lupdate is None:
        return [
            "Qt lupdate was not found; install Qt host tools or pass "
            "--skip-source-scan only for environments that cannot provide them"
        ]

    with tempfile.TemporaryDirectory(prefix="zarya-translations-") as directory:
        generated = Path(directory) / "zarya_source.ts"
        result = subprocess.run(
            [
                str(lupdate),
                str(SRC_DIR),
                "-recursive",
                "-extensions",
                "cpp,h,mm",
                "-locations",
                "none",
                "-ts",
                str(generated),
            ],
            capture_output=True,
            check=False,
            text=True,
        )
        if result.returncode != 0:
            detail = (result.stderr or result.stdout).strip()
            return [f"lupdate source scan failed: {detail or 'unknown error'}"]
        source_pairs = catalog_pairs(generated)

    errors: list[str] = []
    for language, catalog in (("English", TS_EN), ("Russian", TS_RU)):
        if not catalog.is_file():
            continue
        missing = sorted(source_pairs - catalog_pairs(catalog))
        for context, source in missing:
            printable = source.replace("\n", "\\n")
            errors.append(
                f"{language} catalog missing {context!r} source {printable!r}"
            )
    return errors


def check_qm_files() -> list[str]:
    errors: list[str] = []
    for name in ("zarya_en.qm", "zarya_ru.qm"):
        if not (QM_DIR / name).is_file():
            errors.append(f"translations/{name} is missing (run zarya_lrelease)")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--skip-source-scan",
        action="store_true",
        help="skip lupdate catalog completeness scan when Qt host tools are unavailable",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    all_errors: list[str] = []
    all_errors.extend(check_ts_exists())
    all_errors.extend(check_russian_source_strings())
    all_errors.extend(check_source_catalog_coverage(args.skip_source_scan))

    coverage_errors, coverage = check_ru_coverage()
    all_errors.extend(coverage_errors)

    qm_errors = check_qm_files()
    if qm_errors:
        print("Note:", "; ".join(qm_errors))

    print(f"Translation coverage:\n  Russian: {coverage:.0f}%")

    if all_errors:
        print("Translation check failed:", file=sys.stderr)
        for err in all_errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print("Translation checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
