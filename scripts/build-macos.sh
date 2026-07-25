#!/usr/bin/env bash
# Build Zarya on macOS (configure if needed, then cmake --build).
# Qt defaults to Homebrew qt@6; override with CMAKE_PREFIX_PATH or QT_ROOT.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CONFIG="${CONFIG:-Release}"
TARGET="${TARGET:-zarya}"
TEST=0
FORCE=0
JOBS=""

usage() {
  cat <<'EOF'
Usage: build-macos.sh [options]

  --config <name>   Build config (default: Release)
  --target <name>   CMake target (default: zarya)
  --test            Also build and run zarya_stable_hardening_test
  --force           Remove build tree and reconfigure
  -j <N>            Parallel build jobs (passed to cmake --build)
  -h, --help        Show this help

Environment:
  CMAKE_PREFIX_PATH  Qt 6 prefix (preferred)
  QT_ROOT            Qt 6 prefix if CMAKE_PREFIX_PATH is unset
  BUILD_DIR          Build directory (default: <repo>/build)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config) CONFIG="$2"; shift 2 ;;
    --target) TARGET="$2"; shift 2 ;;
    --test) TEST=1; shift ;;
    --force) FORCE=1; shift ;;
    -j) JOBS="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found. Install with: brew install cmake" >&2
  exit 1
fi

resolve_qt_prefix() {
  if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
    # Use the first path entry.
    echo "${CMAKE_PREFIX_PATH%%:*}"
    return
  fi
  if [[ -n "${QT_ROOT:-}" ]]; then
    echo "$QT_ROOT"
    return
  fi
  if command -v brew >/dev/null 2>&1; then
    local prefix
    prefix="$(brew --prefix qt@6 2>/dev/null || true)"
    if [[ -n "$prefix" && -d "$prefix" ]]; then
      echo "$prefix"
      return
    fi
  fi
  return 1
}

QT_PREFIX="$(resolve_qt_prefix)" || {
  echo "Qt 6 not found. Install with: brew install qt@6" >&2
  echo "Or set CMAKE_PREFIX_PATH / QT_ROOT to your Qt prefix." >&2
  exit 1
}

if [[ ! -f "$QT_PREFIX/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
  echo "Qt6Config.cmake not found under $QT_PREFIX" >&2
  echo "Install with: brew install qt@6" >&2
  echo "Or set CMAKE_PREFIX_PATH / QT_ROOT to a valid Qt 6 prefix." >&2
  exit 1
fi

cd "$ROOT"

if [[ "$FORCE" -eq 1 && -d "$BUILD_DIR" ]]; then
  echo "Removing build tree for reconfigure..."
  rm -rf "$BUILD_DIR"
fi

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "No build tree — configuring with Qt at $QT_PREFIX ..."
  cmake -S "$ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCMAKE_PREFIX_PATH="$QT_PREFIX"
fi

BUILD_ARGS=(--build "$BUILD_DIR" --config "$CONFIG" --target "$TARGET")
if [[ -n "$JOBS" ]]; then
  BUILD_ARGS+=(--parallel "$JOBS")
fi

cmake "${BUILD_ARGS[@]}"

if [[ "$TEST" -eq 1 ]]; then
  TEST_BUILD_ARGS=(--build "$BUILD_DIR" --config "$CONFIG" --target zarya_stable_hardening_test)
  if [[ -n "$JOBS" ]]; then
    TEST_BUILD_ARGS+=(--parallel "$JOBS")
  fi
  cmake "${TEST_BUILD_ARGS[@]}"

  TEST_EXE="$BUILD_DIR/zarya_stable_hardening_test"
  [[ -x "$TEST_EXE" ]] || TEST_EXE="$BUILD_DIR/$CONFIG/zarya_stable_hardening_test"
  if [[ ! -x "$TEST_EXE" ]]; then
    echo "Test binary not found: $TEST_EXE" >&2
    exit 1
  fi
  "$TEST_EXE"
fi

APP_PATH="$BUILD_DIR/zarya.app"
[[ -d "$APP_PATH" ]] || APP_PATH="$BUILD_DIR/$CONFIG/zarya.app"

echo ""
echo "Built target: $TARGET ($CONFIG)"
if [[ -d "$APP_PATH" ]]; then
  echo "App bundle:  $APP_PATH"
  echo "Run:         open \"$APP_PATH\""
fi
