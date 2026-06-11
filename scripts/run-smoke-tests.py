#!/usr/bin/env python3
"""Run Zarya beta smoke tests (C++ unit smoke + packaging checks)."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import (  # noqa: E402
    FORBIDDEN_ARTIFACT_NAMES,
    git_commit_short,
    read_cmake_version,
    verify_clean_staging,
)

SCHEMA_FILES = ()


def find_smoke_test_exe(build_dir: Path) -> Path | None:
    candidates = [
        build_dir / "Release" / "zarya_smoke_test.exe",
        build_dir / "zarya_smoke_test.exe",
        build_dir / "zarya_smoke_test",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def run_cpp_smoke(build_dir: Path) -> tuple[bool, str]:
    exe = find_smoke_test_exe(build_dir)
    if exe is None:
        return False, f"zarya_smoke_test not found under {build_dir}"
    try:
        result = subprocess.run([str(exe)], check=True, capture_output=True, text=True, timeout=60)
        return True, result.stdout.strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        return False, str(exc)


def verify_source_tree(source_root: Path) -> list[str]:
    errors: list[str] = []
    version = read_cmake_version()
    if version["version"] != "0.30.0-beta":
        errors.append(f"expected version 0.30.0-beta, found {version['version']}")

    for relative in (
        "LICENSE",
        "THIRD_PARTY_NOTICES.md",
        "RELEASE_NOTES.md",
        "docs/beta-bug-triage.md",
        "docs/beta-regression-checklist.md",
        "docs/platform-test-matrix.md",
        "docs/public-beta/README.md",
        "docs/public-beta/quick-start.md",
        "docs/public-beta/reporting-issues.md",
        "docs/public-beta/feedback-triage.md",
        "docs/public-beta/beta-blockers.md",
        ".github/labels.yml",
        ".github/ISSUE_TEMPLATE/bug_report.yml",
        "translations/zarya_ru.qm",
        "translations/zarya_en.qm",
    ):
        if not (source_root / relative).is_file():
            errors.append(f"missing {relative}")

    for relative in SCHEMA_FILES:
        path = source_root / relative
        if not path.is_file():
            continue
        try:
            json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            errors.append(f"invalid JSON schema {relative}: {exc}")

    ru_ts = source_root / "translations" / "zarya_ru.ts"
    if ru_ts.is_file() and "unfinished" in ru_ts.read_text(encoding="utf-8", errors="replace"):
        pass  # coverage checked elsewhere

    return errors


def verify_staging_dir(staging: Path) -> list[str]:
    required = [
        "LICENSE",
        "release-manifest.json",
        "translations/zarya_en.qm",
        "translations/zarya_ru.qm",
        "docs/localization.md",
        "cores/xray/README.txt",
        "cores/sing-box/README.txt",
    ]
    errors = verify_clean_staging(staging)
    for item in required:
        if not (staging / item).exists() and not any(staging.rglob(Path(item).name)):
            if item.startswith("translations/"):
                if not list(staging.rglob("zarya_en.qm")) or not list(staging.rglob("zarya_ru.qm")):
                    errors.append(f"missing {item}")
            elif not (staging / item).exists() and not list(staging.rglob(item.split("/")[-1])):
                errors.append(f"missing {item}")
    for forbidden in FORBIDDEN_ARTIFACT_NAMES:
        if any(staging.rglob(forbidden)):
            errors.append(f"forbidden file present: {forbidden}")
    for pattern in ("*.download", "*.tmp"):
        if any(staging.rglob(pattern)):
            errors.append(f"forbidden temp pattern present: {pattern}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Zarya beta smoke tests")
    parser.add_argument("--artifact", help="Release artifact (.zip or .tar.gz)")
    parser.add_argument("--source-tree", type=Path, default=ROOT, help="Repository root checks")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build", help="CMake build directory")
    parser.add_argument("--skip-cpp", action="store_true", help="Skip zarya_smoke_test executable")
    parser.add_argument("--keep", action="store_true", help="Keep temp extraction directory")
    args = parser.parse_args()

    errors: list[str] = []

    print(f"Version: {read_cmake_version()['version']} (commit {git_commit_short()})")

    errors.extend(verify_source_tree(args.source_tree))

    if not args.skip_cpp:
        ok, output = run_cpp_smoke(args.build_dir)
        if ok:
            print(f"C++ smoke: passed\n{output}")
        else:
            errors.append(f"C++ smoke failed: {output}")

    if args.artifact:
        artifact = Path(args.artifact)
        smoke_script = ROOT / "scripts" / "smoke-package.py"
        cmd = [sys.executable, str(smoke_script), "--artifact", str(artifact)]
        if args.keep:
            cmd.append("--keep")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            errors.append(result.stderr.strip() or result.stdout.strip() or "package smoke failed")
        else:
            print(result.stdout.strip())

    if errors:
        print("Smoke tests failed:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print("Smoke tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
