#!/usr/bin/env python3
"""Prevent growth of application-owned Qt visual controls during lib_ui migration."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = REPO_ROOT / "src"
DEFAULT_ALLOWLIST = REPO_ROOT / "scripts" / "libui-qt-widget-allowlist.json"

# These files support the centralized native QMessageBox fallback but do not create
# application-owned surfaces. Keep the exclusions exact so direct Qt controls in any
# other application layer are still inventoried.
INFRASTRUCTURE_EXCLUSIONS = {
    "src/app/Application.cpp",
    "src/platform/macos/MacMessageBoxEscape.h",
    "src/platform/macos/MacMessageBoxEscape.mm",
}

# Infrastructure, native integration, approved model/view boundaries, and host-only
# QWidget/QDialog/QMainWindow classes are deliberately absent from this list.
VISUAL_CONTROLS = {
    "QCalendarWidget",
    "QCheckBox",
    "QComboBox",
    "QCommandLinkButton",
    "QDateEdit",
    "QDateTimeEdit",
    "QDial",
    "QDialogButtonBox",
    "QDoubleSpinBox",
    "QFontComboBox",
    "QFrame",
    "QGroupBox",
    "QInputDialog",
    "QLabel",
    "QLCDNumber",
    "QLineEdit",
    "QListView",
    "QListWidget",
    "QMessageBox",
    "QProgressBar",
    "QProgressDialog",
    "QPushButton",
    "QRadioButton",
    "QScrollArea",
    "QSlider",
    "QSpinBox",
    "QStackedWidget",
    "QTabBar",
    "QTabWidget",
    "QTextBrowser",
    "QTimeEdit",
    "QToolBox",
    "QToolButton",
    "QTreeView",
    "QTreeWidget",
    "QWizard",
    "QWizardPage",
}
CONTROL_PATTERN = re.compile(
    r"\b(" + "|".join(sorted(VISUAL_CONTROLS)) + r")\b"
)


def scan() -> dict[str, dict[str, int]]:
    result: dict[str, dict[str, int]] = {}
    for path in sorted(SOURCE_ROOT.rglob("*")):
        if path.suffix not in {".cpp", ".h", ".mm"}:
            continue
        relative_path = path.relative_to(REPO_ROOT).as_posix()
        if relative_path in INFRASTRUCTURE_EXCLUSIONS:
            continue
        counts = Counter(CONTROL_PATTERN.findall(path.read_text(encoding="utf-8")))
        if counts:
            result[relative_path] = dict(sorted(counts.items()))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--allowlist", type=Path, default=DEFAULT_ALLOWLIST)
    parser.add_argument(
        "--previous-git-ref",
        help="also reject allowlist increases compared with this Git revision",
    )
    parser.add_argument(
        "--print-current",
        action="store_true",
        help="print the current inventory instead of checking it",
    )
    args = parser.parse_args()

    current = scan()
    if args.print_current:
        print(json.dumps(current, indent=2, sort_keys=True))
        return 0

    try:
        allowed = json.loads(args.allowlist.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"lib_ui boundary: cannot read {args.allowlist}: {error}", file=sys.stderr)
        return 2

    if args.previous_git_ref:
        relative_allowlist = args.allowlist.resolve().relative_to(REPO_ROOT).as_posix()
        relative_script = Path(__file__).resolve().relative_to(REPO_ROOT).as_posix()
        previous = subprocess.run(
            ["git", "show", f"{args.previous_git_ref}:{relative_allowlist}"],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        if previous.returncode == 0:
            try:
                previous_allowed = json.loads(previous.stdout)
            except json.JSONDecodeError as error:
                print(f"lib_ui boundary: invalid previous allowlist: {error}", file=sys.stderr)
                return 2
            previous_script = subprocess.run(
                ["git", "show", f"{args.previous_git_ref}:{relative_script}"],
                cwd=REPO_ROOT,
                check=False,
                capture_output=True,
                text=True,
            )
            full_source_baseline = (
                previous_script.returncode == 0
                and 'SOURCE_ROOT = REPO_ROOT / "src"' in previous_script.stdout
                and '"QInputDialog"' in previous_script.stdout
                and '"QProgressDialog"' in previous_script.stdout
            )
            if full_source_baseline:
                increases = []
                for path, controls in allowed.items():
                    for control, count in controls.items():
                        old = previous_allowed.get(path, {}).get(control, 0)
                        if count > old:
                            increases.append((path, control, old, count))
                if increases:
                    print("lib_ui boundary allowlist must only shrink:", file=sys.stderr)
                    for path, control, old, new in increases:
                        print(f"  {path}: {control} {old} -> {new}", file=sys.stderr)
                    return 1
        elif "exists on disk, but not in" not in previous.stderr:
            print(previous.stderr.strip(), file=sys.stderr)
            return 2

    if current == allowed:
        total = sum(sum(counts.values()) for counts in current.values())
        print(f"lib_ui boundary: OK ({total} legacy control references)")
        return 0

    current_entries = {
        (path, control): count
        for path, controls in current.items()
        for control, count in controls.items()
    }
    allowed_entries = {
        (path, control): count
        for path, controls in allowed.items()
        for control, count in controls.items()
    }
    print("lib_ui boundary inventory changed:", file=sys.stderr)
    for entry in sorted(current_entries.keys() | allowed_entries.keys()):
        old = allowed_entries.get(entry, 0)
        new = current_entries.get(entry, 0)
        if old != new:
            marker = "increased" if new > old else "decreased"
            print(
                f"  {entry[0]}: {entry[1]} {old} -> {new} ({marker})",
                file=sys.stderr,
            )
    print(
        "Migrations must reduce this inventory. After reviewing the diff, update "
        "scripts/libui-qt-widget-allowlist.json with --print-current.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
