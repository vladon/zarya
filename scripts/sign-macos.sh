#!/usr/bin/env bash
set -euo pipefail

APP_PATH=""
IDENTITY=""
NOTARIZE=0
DRY_RUN=0
APPLE_ID=""
TEAM_ID=""
PASSWORD=""
API_KEY=""
API_ISSUER=""

usage() {
  cat <<'EOF'
Usage: sign-macos.sh --app-path Zarya.app [options]

Options:
  --identity <name>     Developer ID Application identity
  --notarize            Submit for notarization (requires credentials)
  --apple-id <id>       Apple ID for notarytool
  --team-id <id>        Apple Team ID
  --password <secret>   App-specific password
  --api-key <path>      App Store Connect API key (.p8)
  --api-issuer <uuid>   API key issuer ID
  --dry-run             Print actions without signing
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --app-path) APP_PATH="$2"; shift 2 ;;
    --identity) IDENTITY="$2"; shift 2 ;;
    --notarize) NOTARIZE=1; shift ;;
    --apple-id) APPLE_ID="$2"; shift 2 ;;
    --team-id) TEAM_ID="$2"; shift 2 ;;
    --password) PASSWORD="$2"; shift 2 ;;
    --api-key) API_KEY="$2"; shift 2 ;;
    --api-issuer) API_ISSUER="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 2 ;;
  esac
done

if [[ -z "$APP_PATH" || ! -d "$APP_PATH" ]]; then
  echo "Missing or invalid --app-path" >&2
  exit 1
fi

if [[ -z "$IDENTITY" ]]; then
  echo "No signing identity provided; dry-run only."
  DRY_RUN=1
fi

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "DRY RUN: would codesign --deep --force --options runtime --sign \"$IDENTITY\" \"$APP_PATH\""
  codesign --verify --deep --strict --verbose=2 "$APP_PATH" 2>/dev/null || echo "DRY RUN: app is currently unsigned"
  if [[ "$NOTARIZE" -eq 1 ]]; then
    echo "DRY RUN: would notarize $APP_PATH"
  fi
  exit 0
fi

codesign --deep --force --options runtime --sign "$IDENTITY" "$APP_PATH"
codesign --verify --deep --strict --verbose=2 "$APP_PATH"

if [[ "$NOTARIZE" -eq 0 ]]; then
  exit 0
fi

ZIP_FOR_NOTARY="$(mktemp -t zarya-notarize.XXXXXX.zip)"
ditto -c -k --keepParent "$APP_PATH" "$ZIP_FOR_NOTARY"

if [[ -n "$API_KEY" && -n "$API_ISSUER" ]]; then
  xcrun notarytool submit "$ZIP_FOR_NOTARY" --wait --key "$API_KEY" --issuer "$API_ISSUER"
else
  if [[ -z "$APPLE_ID" || -z "$TEAM_ID" || -z "$PASSWORD" ]]; then
    echo "Notarization requires --api-key/--api-issuer or --apple-id/--team-id/--password" >&2
    rm -f "$ZIP_FOR_NOTARY"
    exit 1
  fi
  xcrun notarytool submit "$ZIP_FOR_NOTARY" --wait --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD"
fi

xcrun stapler staple "$APP_PATH"
spctl --assess --type execute --verbose=4 "$APP_PATH" || true
rm -f "$ZIP_FOR_NOTARY"
