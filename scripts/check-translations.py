#!/usr/bin/env python3
"""Lightweight translation checks for CI and local development."""

from __future__ import annotations

import re
import sys
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


def check_qm_files() -> list[str]:
    errors: list[str] = []
    for name in ("zarya_en.qm", "zarya_ru.qm"):
        if not (QM_DIR / name).is_file():
            errors.append(f"translations/{name} is missing (run zarya_lrelease)")
    return errors


def main() -> int:
    all_errors: list[str] = []
    all_errors.extend(check_ts_exists())
    all_errors.extend(check_russian_source_strings())

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
