#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="0.12.0"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
cmake -S "$ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target zarya
BIN="$BUILD_DIR/zarya"
[[ -x "$BIN" ]] || BIN="$BUILD_DIR/Release/zarya"
STAGING="$ROOT/dist/Zarya-${VERSION}-linux-x64"
rm -rf "$STAGING"
mkdir -p "$STAGING/cores/xray"
cp "$BIN" "$STAGING/zarya"
cp "$ROOT/packaging/windows/cores-xray-README.txt" "$STAGING/cores/xray/README.txt"
cp "$ROOT/packaging/linux/zarya.desktop.in" "$STAGING/zarya.desktop"
cp "$ROOT/README.md" "$STAGING/" 2>/dev/null || true
cp "$ROOT/LICENSE" "$STAGING/" 2>/dev/null || true
TARBALL="$ROOT/dist/Zarya-${VERSION}-linux-x64.tar.gz"
tar -czf "$TARBALL" -C "$ROOT/dist" "Zarya-${VERSION}-linux-x64"
echo "Created $TARBALL"
