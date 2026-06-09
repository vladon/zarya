#!/usr/bin/env python3
"""Apply Russian translations from ru_translations.json to translations/zarya_ru.ts."""

from __future__ import annotations

import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TS_PATH = ROOT / "translations" / "zarya_ru.ts"
JSON_PATH = Path(__file__).resolve().parent / "ru_translations.json"

# Qt Russian plural forms (n % 10 == 1 && n % 100 != 11, etc.)
PLURAL_FORMS: dict[str, tuple[str, str, str]] = {
    "%1 subscription(s) failed to update.": (
        "%1 подписка не обновлена.",
        "%1 подписки не обновлены.",
        "%1 подписок не обновлено.",
    ),
    "Imported %n profile(s). Some lines failed:": (
        "Импортирован %n профиль. Некоторые строки не удались:",
        "Импортировано %n профиля. Некоторые строки не удались:",
        "Импортировано %n профилей. Некоторые строки не удались:",
    ),
}


def source_text(msg: ET.Element) -> str:
    src = msg.find("source")
    if src is None:
        return ""
    return "".join(src.itertext())


def apply_plural_translation(msg: ET.Element, forms: tuple[str, str, str]) -> None:
    for child in list(msg):
        if child.tag == "translation":
            msg.remove(child)
    tr = ET.SubElement(msg, "translation")
    for form in forms:
        ET.SubElement(tr, "numerusform").text = form
    msg.set("numerus", "yes")


def apply_simple_translation(msg: ET.Element, text: str) -> None:
    tr = msg.find("translation")
    if tr is None:
        tr = ET.SubElement(msg, "translation")
    else:
        tr.attrib.pop("type", None)
        tr.clear()
    tr.text = text


def main() -> int:
    if not JSON_PATH.is_file():
        print(f"Missing {JSON_PATH}", file=sys.stderr)
        return 1
    if not TS_PATH.is_file():
        print(f"Missing {TS_PATH}", file=sys.stderr)
        return 1

    translations: dict[str, str] = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    tree = ET.parse(TS_PATH)
    root = tree.getroot()

    translated = 0
    missing: list[str] = []

    for ctx in root.findall("context"):
        for msg in ctx.findall("message"):
            src = source_text(msg)
            if not src:
                continue
            if src in PLURAL_FORMS:
                apply_plural_translation(msg, PLURAL_FORMS[src])
                translated += 1
                continue
            ru = translations.get(src)
            if ru is None:
                missing.append(src)
                continue
            apply_simple_translation(msg, ru)
            translated += 1

    tree.write(TS_PATH, encoding="utf-8", xml_declaration=True)
    print(f"Applied {translated} translations")
    if missing:
        print(f"Missing {len(missing)} strings:", file=sys.stderr)
        for item in missing[:20]:
            print(f"  - {item!r}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
