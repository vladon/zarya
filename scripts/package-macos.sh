#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="0.16.0"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
cmake -S "$ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --config Release --target zarya zarya-helper
APP_PATH="$BUILD_DIR/zarya.app"
if [[ ! -d "$APP_PATH" ]]; then
  APP_PATH="$BUILD_DIR/Release/zarya.app"
fi
if command -v macdeployqt >/dev/null 2>&1 && [[ -d "$APP_PATH" ]]; then
  macdeployqt "$APP_PATH"
fi
DIST="$ROOT/dist"
mkdir -p "$DIST"
ZIP="$DIST/Zarya-${VERSION}-macos.zip"
rm -f "$ZIP"
(cd "$(dirname "$APP_PATH")" && zip -r "$ZIP" "$(basename "$APP_PATH")")
echo "Created $ZIP"
