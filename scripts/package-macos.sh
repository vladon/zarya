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

SIGNING_IDENTITY=""

NOTARIZE=0

APPLE_ID=""

TEAM_ID=""

NOTARY_PASSWORD=""

API_KEY=""

API_ISSUER=""



usage() {

  cat <<'EOF'

Usage: package-macos.sh [options]



  --sign                  Enable codesign (optional notarization with --notarize)

  --skip-signing          Skip signing (default)

  --signing-identity <id> Developer ID Application identity

  --notarize              Submit for notarization when signing

  --apple-id <id>         Apple ID for notarytool

  --team-id <id>          Apple Team ID

  --password <secret>     App-specific password

  --api-key <path>        App Store Connect API key

  --api-issuer <uuid>     API key issuer ID

EOF

}



while [[ $# -gt 0 ]]; do

  case "$1" in

    --build-dir) BUILD_DIR="$2"; shift 2 ;;

    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;

    --arch) ARCH="$2"; shift 2 ;;

    --skip-build) SKIP_BUILD=1; shift ;;

    --skip-tests) SKIP_TESTS=1; shift ;;

    --sign) SIGN=1; shift ;;

    --skip-signing) SKIP_SIGNING=1; shift ;;

    --signing-identity) SIGNING_IDENTITY="$2"; shift 2 ;;

    --notarize) NOTARIZE=1; shift ;;

    --apple-id) APPLE_ID="$2"; shift 2 ;;

    --team-id) TEAM_ID="$2"; shift 2 ;;

    --password) NOTARY_PASSWORD="$2"; shift 2 ;;

    --api-key) API_KEY="$2"; shift 2 ;;

    --api-issuer) API_ISSUER="$2"; shift 2 ;;

    -h|--help) usage; exit 0 ;;

    *) echo "Unknown option: $1" >&2; exit 2 ;;

  esac

done



if [[ "$SKIP_SIGNING" -eq 1 ]]; then

  SIGN=0

fi



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

from release_common import copy_docs, copy_installer_docs, copy_public_beta_docs, copy_stable_docs, copy_translations, copy_updater_docs, create_placeholder_layout, write_build_integrity



resources = Path("$RESOURCES_DIR")

copy_docs(resources)

copy_public_beta_docs(resources)

copy_installer_docs(resources)

copy_updater_docs(resources)

copy_stable_docs(resources)

copy_translations(resources, Path("$BUILD_DIR/translations"))

create_placeholder_layout(resources)

write_build_integrity(resources)

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



if [[ "$SIGN" -eq 1 ]]; then

  SIGN_ARGS=(--app-path "$APP_PATH")

  [[ -n "$SIGNING_IDENTITY" ]] && SIGN_ARGS+=(--identity "$SIGNING_IDENTITY")

  [[ "$NOTARIZE" -eq 1 ]] && SIGN_ARGS+=(--notarize)

  [[ -n "$APPLE_ID" ]] && SIGN_ARGS+=(--apple-id "$APPLE_ID")

  [[ -n "$TEAM_ID" ]] && SIGN_ARGS+=(--team-id "$TEAM_ID")

  [[ -n "$NOTARY_PASSWORD" ]] && SIGN_ARGS+=(--password "$NOTARY_PASSWORD")

  [[ -n "$API_KEY" ]] && SIGN_ARGS+=(--api-key "$API_KEY")

  [[ -n "$API_ISSUER" ]] && SIGN_ARGS+=(--api-issuer "$API_ISSUER")

  bash "$ROOT/scripts/sign-macos.sh" "${SIGN_ARGS[@]}"

  export ZARYA_PACKAGE_SIGNED="macos-developer-id"

  [[ "$NOTARIZE" -eq 1 ]] && export ZARYA_PACKAGE_NOTARIZED=1

else

  echo "No signing requested; skipping signing step"

  unset ZARYA_PACKAGE_SIGNED ZARYA_PACKAGE_NOTARIZED

fi



python3 - <<PY
import os
import sys
from pathlib import Path

ROOT = Path("$ROOT")
sys.path.insert(0, str(ROOT / "scripts"))
from release_common import build_signing_manifest, default_unsigned_signing, update_manifest_signing, write_build_integrity

resources = Path("$RESOURCES_DIR")

mode = os.environ.get("ZARYA_PACKAGE_SIGNED", "")

if mode == "macos-developer-id":

    signing = build_signing_manifest(

        signed=True,

        signature_type="macos-developer-id",

        notarized=os.environ.get("ZARYA_PACKAGE_NOTARIZED") == "1",

        verification={

            "macosCodesign": "valid",

            "macosNotarization": "stapled" if os.environ.get("ZARYA_PACKAGE_NOTARIZED") == "1" else "not submitted",

        },

    )

else:

    signing = default_unsigned_signing()

update_manifest_signing(resources, signing)
write_build_integrity(resources, signing)
PY

cp "$RESOURCES_DIR/build-integrity.json" "$MACOS_DIR/build-integrity.json" 2>/dev/null || true



mkdir -p "$OUTPUT_DIR"

ZIP_PATH="$OUTPUT_DIR/$ARTIFACT_NAME"

rm -f "$ZIP_PATH"

(cd "$(dirname "$APP_PATH")" && zip -r "$ZIP_PATH" "$(basename "$APP_PATH")")

python3 -c "import sys; sys.path.insert(0, '$ROOT/scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path('$OUTPUT_DIR'), Path('$ZIP_PATH'))"

echo "Created $ZIP_PATH"



if [[ "$SKIP_TESTS" -eq 0 ]]; then

  python3 "$ROOT/scripts/smoke-package.py" --artifact "$ZIP_PATH" || true

  python3 "$ROOT/scripts/verify-release-artifacts.py" --artifact "$ZIP_PATH" --expected-version "$VERSION" --require-checksum --allow-unsigned

fi

