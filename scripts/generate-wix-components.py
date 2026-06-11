#!/usr/bin/env python3
"""Generate WiX v4 component fragments from an MSI staging directory."""

from __future__ import annotations

import argparse
import re
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

SKIP_FILES: set[str] = set()


def stable_guid(relative: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, f"zarya-msi:{relative}"))


def sanitize_id(relative: str) -> str:
    token = re.sub(r"[^A-Za-z0-9_]", "_", relative.replace("\\", "_").replace("/", "_"))
    if not token or token[0].isdigit():
        token = f"f_{token}"
    return token


def directory_id_for(parts: tuple[str, ...]) -> str:
    if not parts:
        return "INSTALLFOLDER"
    return "dir_" + "_".join(sanitize_id(part) for part in parts)


def collect_directories(files: list[Path], staging: Path) -> dict[tuple[str, ...], str]:
    dirs: dict[tuple[str, ...], str] = {(): "INSTALLFOLDER"}
    for file_path in files:
        rel = file_path.relative_to(staging)
        parts = rel.parts[:-1]
        for i in range(len(parts)):
            key = tuple(parts[: i + 1])
            dirs.setdefault(key, directory_id_for(key))
    return dict(sorted(dirs.items(), key=lambda item: item[0]))


def render_directories_fragment(dirs: dict[tuple[str, ...], str]) -> str:
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">',
        "  <Fragment>",
        '    <DirectoryRef Id="INSTALLFOLDER">',
    ]
    children: dict[str, list[tuple[str, str]]] = {}
    for parts, dir_id in dirs.items():
        if not parts:
            continue
        parent = directory_id_for(parts[:-1]) if len(parts) > 1 else "INSTALLFOLDER"
        children.setdefault(parent, []).append((dir_id, parts[-1]))

    def walk(parent_id: str, indent: int) -> None:
        for dir_id, name in sorted(children.get(parent_id, []), key=lambda item: item[1]):
            lines.append(f'{" " * indent}<Directory Id="{dir_id}" Name="{name}">')
            walk(dir_id, indent + 2)
            lines.append(f'{" " * indent}</Directory>')

    walk("INSTALLFOLDER", 6)
    lines.extend(
        [
            "    </DirectoryRef>",
            "  </Fragment>",
            "</Wix>",
            "",
        ]
    )
    return "\n".join(lines)


def render_components_fragment(files: list[Path], staging: Path) -> str:
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">',
        "  <Fragment>",
        '    <ComponentGroup Id="ZaryaAppComponents">',
    ]
    for file_path in sorted(files):
        rel = file_path.relative_to(staging).as_posix()
        if rel in SKIP_FILES:
            continue
        parts = file_path.relative_to(staging).parts
        dir_id = directory_id_for(parts[:-1])
        comp_id = f"cmp_{sanitize_id(rel)}"
        file_id = f"file_{sanitize_id(rel)}"
        guid = stable_guid(rel)
        lines.append(f'      <Component Id="{comp_id}" Directory="{dir_id}" Guid="{guid}">')
        lines.append(
            f'        <File Id="{file_id}" Source="!(bindpath.staging)\\{rel}" KeyPath="yes" />'
        )
        if rel.lower() == "zarya-helper.exe":
            lines.extend(
                [
                    '        <ServiceInstall Id="ZaryaHelperServiceInstall"',
                    '          Name="ZaryaHelper"',
                    '          DisplayName="Zarya Helper"',
                    '          Description="Privileged helper service for Zarya experimental TUN and kill switch features."',
                    '          Type="ownProcess"',
                    '          Start="demand"',
                    '          ErrorControl="normal"',
                    '          Account="LocalSystem"',
                    '          Arguments="--service --service-name ZaryaHelper"',
                    '          Vital="yes"',
                    '          Condition="INSTALLHELPERSERVICE=&quot;1&quot;" />',
                    '        <ServiceControl Id="ZaryaHelperServiceControl"',
                    '          Name="ZaryaHelper"',
                    '          Stop="both"',
                    '          Remove="uninstall"',
                    '          Wait="yes"',
                    '          Condition="INSTALLHELPERSERVICE=&quot;1&quot;" />',
                ]
            )
        lines.append("      </Component>")
    lines.extend(["    </ComponentGroup>", "  </Fragment>", "</Wix>", ""])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate WiX components from staging directory")
    parser.add_argument("--staging", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "packaging" / "windows" / "wix" / "generated")
    args = parser.parse_args()

    staging = args.staging.resolve()
    if not staging.is_dir():
        raise SystemExit(f"staging directory not found: {staging}")

    files = [path for path in staging.rglob("*") if path.is_file()]
    dirs = collect_directories(files, staging)
    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    (output_dir / "GeneratedDirectories.wxs").write_text(
        render_directories_fragment(dirs), encoding="utf-8"
    )
    (output_dir / "GeneratedComponents.wxs").write_text(
        render_components_fragment(files, staging), encoding="utf-8"
    )
    print(f"Generated WiX fragments in {output_dir} ({len(files)} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
