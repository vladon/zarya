#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="0.17.0"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
cmake -S "$ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target zarya zarya-helper
BIN="$BUILD_DIR/zarya"
[[ -x "$BIN" ]] || BIN="$BUILD_DIR/Release/zarya"
HELPER_BIN="$BUILD_DIR/zarya-helper"
[[ -x "$HELPER_BIN" ]] || HELPER_BIN="$BUILD_DIR/Release/zarya-helper"
STAGING="$ROOT/dist/Zarya-${VERSION}-linux-x64"
rm -rf "$STAGING"
mkdir -p "$STAGING/cores/xray"
cp "$BIN" "$STAGING/zarya"
if [[ -x "$HELPER_BIN" ]]; then
  cp "$HELPER_BIN" "$STAGING/zarya-helper"
fi
cp "$ROOT/packaging/windows/cores-xray-README.txt" "$STAGING/cores/xray/README.txt"
cp "$ROOT/packaging/linux/zarya.desktop.in" "$STAGING/zarya.desktop"
cp "$ROOT/README.md" "$STAGING/" 2>/dev/null || true
cp "$ROOT/LICENSE" "$STAGING/" 2>/dev/null || true
TARBALL="$ROOT/dist/Zarya-${VERSION}-linux-x64.tar.gz"
tar -czf "$TARBALL" -C "$ROOT/dist" "Zarya-${VERSION}-linux-x64"
echo "Created $TARBALL"
