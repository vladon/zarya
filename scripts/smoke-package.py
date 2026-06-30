#!/usr/bin/env python3
"""Cross-platform packaging smoke checks."""

from __future__ import annotations

import argparse
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from release_common import (  # noqa: E402
    DOC_FILES,
    FORBIDDEN_ARTIFACT_NAMES,
    artifact_content_root,
    extract_tar_gz,
    extract_zip,
    run_version_check,
    verify_clean_staging,
)


def find_staging_root(extracted: Path) -> Path:
    children = [p for p in extracted.iterdir() if p.name != "__MACOSX"]
    if len(children) == 1 and children[0].is_dir():
        return children[0]
    return extracted


def locate_executables(staging: Path) -> tuple[Path | None, Path | None, Path | None]:
    gui_candidates = list(staging.rglob("Zarya.exe")) + list(staging.rglob("zarya")) + list(
        staging.rglob("Zarya")
    )
    helper_candidates = list(staging.rglob("zarya-helper.exe")) + list(staging.rglob("zarya-helper"))
    updater_candidates = list(staging.rglob("zarya-updater.exe")) + list(
        staging.rglob("zarya-updater")
    )
    gui = next((p for p in gui_candidates if p.is_file()), None)
    helper = next((p for p in helper_candidates if p.is_file()), None)
    updater = next((p for p in updater_candidates if p.is_file()), None)
    return gui, helper, updater


def main() -> int:
    parser = argparse.ArgumentParser(description="Smoke-test a Zarya release artifact")
    parser.add_argument("--artifact", required=True, help="Path to .zip or .tar.gz")
    parser.add_argument("--keep", action="store_true", help="Keep temp extraction directory")
    args = parser.parse_args()

    artifact = Path(args.artifact)
    if not artifact.is_file():
        print(f"artifact not found: {artifact}", file=sys.stderr)
        return 1

    temp_dir = Path(tempfile.mkdtemp(prefix="zarya-smoke-"))
    errors: list[str] = []
    try:
        if artifact.suffix == ".zip":
            extract_zip(artifact, temp_dir)
        elif artifact.name.endswith(".tar.gz"):
            extract_tar_gz(artifact, temp_dir)
        else:
            print(f"unsupported artifact type: {artifact}", file=sys.stderr)
            return 1

        staging = find_staging_root(temp_dir)
        content = artifact_content_root(staging)
        errors.extend(verify_clean_staging(staging))

        manifest = content / "release-manifest.json"
        if not manifest.is_file():
            manifest = next(staging.rglob("release-manifest.json"), None)
        if manifest is None or not manifest.is_file():
            errors.append("release-manifest.json is missing")

        license_path = content / "LICENSE"
        if not license_path.is_file():
            license_path = next(staging.rglob("LICENSE"), None)
        if license_path is None or not license_path.is_file():
            errors.append("LICENSE is missing")

        translations = content / "translations"
        if not translations.is_dir():
            translations = next((p for p in staging.rglob("translations") if p.is_dir()), None)
        if translations is None or not any(translations.glob("*.qm")):
            errors.append("translations/*.qm missing")

        docs_dir = content / "docs"
        if not docs_dir.is_dir():
            docs_dir = next((p for p in staging.rglob("docs") if p.is_dir()), None)
        if docs_dir is None:
            errors.append("docs/ directory missing")
        else:
            for doc in DOC_FILES:
                if not (docs_dir / doc).is_file():
                    errors.append(f"docs/{doc} missing")

        for forbidden in FORBIDDEN_ARTIFACT_NAMES:
            if any(staging.rglob(forbidden)):
                errors.append(f"forbidden file present: {forbidden}")

        gui, helper, updater = locate_executables(staging)
        if gui is None:
            errors.append("GUI executable not found")
        if helper is None:
            errors.append("zarya-helper executable not found")
        if updater is None:
            errors.append("zarya-updater executable not found")

        if gui is not None:
            ok, output = run_version_check(gui, gui=True)
            if not ok:
                errors.append(f"--version failed for GUI: {output}")
            elif "Zarya" not in output:
                errors.append(f"unexpected --version output: {output}")

        if helper is not None:
            ok, output = run_version_check(helper)
            if not ok:
                errors.append(f"--version failed for helper: {output}")
            elif "zarya-helper" not in output:
                errors.append(f"unexpected helper --version output: {output}")

        if updater is not None:
            ok, output = run_version_check(updater)
            if not ok:
                errors.append(f"--version failed for updater: {output}")
            elif "zarya-updater" not in output:
                errors.append(f"unexpected updater --version output: {output}")

        if errors:
            print("Smoke test failed:", file=sys.stderr)
            for err in errors:
                print(f"  - {err}", file=sys.stderr)
            return 1

        print(f"Smoke test passed for {artifact.name}")
        return 0
    finally:
        if not args.keep:
            shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
