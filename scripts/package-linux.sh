#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT/dist}"
SKIP_BUILD=0
SKIP_TESTS=0
ARCH="${ARCH:-}"
SIGN=0
SKIP_SIGNING=0
GPG_SIGN=0
MINISIGN=0
SIGNING_KEY=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
    --arch) ARCH="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-tests) SKIP_TESTS=1; shift ;;
    --sign) SIGN=1; shift ;;
    --skip-signing) SKIP_SIGNING=1; shift ;;
    --gpg-sign) GPG_SIGN=1; SIGN=1; shift ;;
    --minisign) MINISIGN=1; SIGN=1; shift ;;
    --key) SIGNING_KEY="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

if [[ "$SKIP_SIGNING" -eq 1 ]]; then
  SIGN=0
  GPG_SIGN=0
  MINISIGN=0
fi

if [[ -z "$ARCH" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    *) ARCH="x64" ;;
  esac
fi

read -r VERSION CHANNEL < <(python3 -c "import sys; sys.path.insert(0, '$ROOT/scripts'); from release_common import read_cmake_version; m=read_cmake_version(); print(m['version'], m['channel'])")
ARTIFACT_BASE="Zarya-${VERSION}-linux-${ARCH}"
STAGING="$OUTPUT_DIR/$ARTIFACT_BASE"
TARBALL="$OUTPUT_DIR/$ARTIFACT_BASE.tar.gz"

if [[ "$SKIP_BUILD" -eq 0 ]]; then
  cmake -S "$ROOT" -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR" --target zarya_lrelease zarya zarya-helper
fi

BIN="$BUILD_DIR/zarya"
[[ -x "$BIN" ]] || BIN="$BUILD_DIR/Release/zarya"
HELPER_BIN="$BUILD_DIR/zarya-helper"
[[ -x "$HELPER_BIN" ]] || HELPER_BIN="$BUILD_DIR/Release/zarya-helper"

rm -rf "$STAGING"
mkdir -p "$STAGING"

cp "$BIN" "$STAGING/zarya"
chmod +x "$STAGING/zarya"
cp "$HELPER_BIN" "$STAGING/zarya-helper"
chmod +x "$STAGING/zarya-helper"
touch "$STAGING/portable.flag"

cp "$ROOT/packaging/linux/zarya.desktop.in" "$STAGING/zarya.desktop"
if [[ -f "$ROOT/packaging/linux/zarya.png" ]]; then
  cp "$ROOT/packaging/linux/zarya.png" "$STAGING/zarya.png"
fi

python3 - <<PY
import sys
from pathlib import Path
sys.path.insert(0, "$ROOT/scripts")
from release_common import (
    copy_top_level_legal_files,
    copy_docs,
    copy_translations,
    create_placeholder_layout,
    write_release_manifest,
    verify_clean_staging,
    write_build_integrity,
)

staging = Path("$STAGING")
build_translations = Path("$BUILD_DIR/translations")
copy_top_level_legal_files(staging)
copy_docs(staging)
copy_translations(staging, build_translations)
create_placeholder_layout(staging)
write_release_manifest(
    staging,
    platform="linux",
    architecture="$ARCH",
    portable=True,
    gui_artifact="zarya",
    helper_artifact="zarya-helper",
)
write_build_integrity(staging)
errors = verify_clean_staging(staging)
if errors:
    raise SystemExit("\\n".join(errors))
PY

if [[ "$SIGN" -eq 1 && ( "$GPG_SIGN" -eq 1 || "$MINISIGN" -eq 1 ) && -n "$SIGNING_KEY" ]]; then
  python3 - <<PY
import sys
from pathlib import Path
sys.path.insert(0, "$ROOT/scripts")
from release_common import build_signing_manifest, update_manifest_signing, write_build_integrity

staging = Path("$STAGING")
verification = {}
if $GPG_SIGN:
    verification["linuxGpg"] = "pending"
if $MINISIGN:
    verification["linuxMinisign"] = "pending"
signing = build_signing_manifest(
    signed=True,
    signature_type="linux-gpg" if $GPG_SIGN else "linux-minisign",
    verification=verification,
)
update_manifest_signing(staging, signing)
write_build_integrity(staging, signing)
PY
elif [[ "$SIGN" -eq 1 ]]; then
  echo "Signing requested but no --key provided; tarball will remain checksum-only."
else
  echo "No signing requested; skipping signing step"
fi

mkdir -p "$OUTPUT_DIR"
rm -f "$TARBALL"
tar -czf "$TARBALL" -C "$OUTPUT_DIR" "$ARTIFACT_BASE"
python3 -c "import sys; sys.path.insert(0, '$ROOT/scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path('$OUTPUT_DIR'), Path('$TARBALL'))"

if [[ "$SIGN" -eq 1 ]]; then
  SIGN_ARGS=(--artifact "$TARBALL")
  [[ "$GPG_SIGN" -eq 1 ]] && SIGN_ARGS+=(--gpg)
  [[ "$MINISIGN" -eq 1 ]] && SIGN_ARGS+=(--minisign)
  [[ -n "$SIGNING_KEY" ]] && SIGN_ARGS+=(--key "$SIGNING_KEY")
  bash "$ROOT/scripts/sign-linux.sh" "${SIGN_ARGS[@]}" || echo "warning: optional Linux signing step skipped (missing tools or key)" >&2
fi

echo "Created $TARBALL"

if [[ "$SKIP_TESTS" -eq 0 ]]; then
  python3 "$ROOT/scripts/smoke-package.py" --artifact "$TARBALL"
  python3 "$ROOT/scripts/verify-release-artifacts.py" --artifact "$TARBALL" --expected-version "$VERSION" --require-checksum --allow-unsigned
fi
