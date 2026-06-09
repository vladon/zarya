#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT/dist}"
SKIP_BUILD=0
SKIP_TESTS=0
ARCH="${ARCH:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
    --arch) ARCH="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-tests) SKIP_TESTS=1; shift ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$ARCH" ]]; then
  case "$(uname -m)" in
    arm64) ARCH="arm64" ;;
    *) ARCH="x64" ;;
  esac
fi

read -r VERSION CHANNEL < <(python3 -c "import sys; sys.path.insert(0, '$ROOT/scripts'); from release_common import read_cmake_version; m=read_cmake_version(); print(m['version'], m['channel'])")
ARTIFACT_NAME="Zarya-${VERSION}-macos-${ARCH}.zip"
APP_PATH="$BUILD_DIR/zarya.app"
[[ -d "$APP_PATH" ]] || APP_PATH="$BUILD_DIR/Release/zarya.app"

if [[ "$SKIP_BUILD" -eq 0 ]]; then
  cmake -S "$ROOT" -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR" --config Release --target zarya_lrelease zarya zarya-helper
fi

if [[ ! -d "$APP_PATH" ]]; then
  echo "zarya.app not found" >&2
  exit 1
fi

HELPER_BIN="$BUILD_DIR/zarya-helper"
[[ -x "$HELPER_BIN" ]] || HELPER_BIN="$BUILD_DIR/Release/zarya-helper"

MACOS_DIR="$APP_PATH/Contents/MacOS"
RESOURCES_DIR="$APP_PATH/Contents/Resources"
mkdir -p "$MACOS_DIR" "$RESOURCES_DIR/translations" "$RESOURCES_DIR/docs" "$RESOURCES_DIR/cores/xray" "$RESOURCES_DIR/cores/sing-box"

if [[ -x "$HELPER_BIN" ]]; then
  cp "$HELPER_BIN" "$MACOS_DIR/zarya-helper"
  chmod +x "$MACOS_DIR/zarya-helper"
fi

python3 - <<PY
import sys
from pathlib import Path
sys.path.insert(0, "$ROOT/scripts")
from release_common import copy_docs, copy_translations, create_placeholder_layout

resources = Path("$RESOURCES_DIR")
staging = resources
copy_docs(staging)
copy_translations(staging, Path("$BUILD_DIR/translations"))
create_placeholder_layout(staging)
PY

for file in README.md LICENSE THIRD_PARTY_NOTICES.md RELEASE_NOTES.md; do
  if [[ -f "$ROOT/$file" ]]; then
    cp "$ROOT/$file" "$RESOURCES_DIR/$file"
  fi
done

touch "$RESOURCES_DIR/portable.flag"

python3 - <<PY
import sys
from pathlib import Path
sys.path.insert(0, "$ROOT/scripts")
from release_common import write_release_manifest

write_release_manifest(
    Path("$RESOURCES_DIR"),
    platform="macos",
    architecture="$ARCH",
    portable=True,
    gui_artifact="Zarya.app/Contents/MacOS/Zarya",
    helper_artifact="Zarya.app/Contents/MacOS/zarya-helper" if Path("$MACOS_DIR/zarya-helper").exists() else None,
)
PY

if command -v macdeployqt >/dev/null 2>&1; then
  macdeployqt "$APP_PATH"
else
  echo "warning: macdeployqt not found" >&2
fi

mkdir -p "$OUTPUT_DIR"
ZIP_PATH="$OUTPUT_DIR/$ARTIFACT_NAME"
rm -f "$ZIP_PATH"
(cd "$(dirname "$APP_PATH")" && zip -r "$ZIP_PATH" "$(basename "$APP_PATH")")
python3 -c "import sys; sys.path.insert(0, '$ROOT/scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path('$OUTPUT_DIR'), Path('$ZIP_PATH'))"
echo "Created $ZIP_PATH"

if [[ "$SKIP_TESTS" -eq 0 ]]; then
  python3 "$ROOT/scripts/smoke-package.py" --artifact "$ZIP_PATH" || true
fi
