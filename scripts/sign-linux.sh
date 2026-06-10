#!/usr/bin/env bash
set -euo pipefail

ARTIFACT=""
GPG=0
MINISIGN=0
KEY=""
DRY_RUN=0
OUTPUT_DIR=""

usage() {
  cat <<'EOF'
Usage: sign-linux.sh --artifact <file> [options]

Options:
  --gpg                 Create detached GPG signature (.asc)
  --minisign            Create minisign signature (.minisig)
  --key <id-or-path>    GPG key id or minisign key path
  --output-dir <dir>    Directory for SHA256SUMS.txt (default: artifact dir)
  --dry-run             Print actions only
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --artifact) ARTIFACT="$2"; shift 2 ;;
    --gpg) GPG=1; shift ;;
    --minisign) MINISIGN=1; shift ;;
    --key) KEY="$2"; shift 2 ;;
    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
    --dry-run) DRY_RUN=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 2 ;;
  esac
done

if [[ -z "$ARTIFACT" || ! -f "$ARTIFACT" ]]; then
  echo "Missing or invalid --artifact" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="${OUTPUT_DIR:-$(dirname "$ARTIFACT")}"

python3 -c "
import sys
sys.path.insert(0, '$ROOT/scripts')
from pathlib import Path
from release_common import write_checksum_sidecars
write_checksum_sidecars(Path('$OUT_DIR'), Path('$ARTIFACT'))
"

if [[ "$DRY_RUN" -eq 1 ]]; then
  echo "DRY RUN: SHA256 checksum written for $ARTIFACT"
  [[ "$GPG" -eq 1 ]] && echo "DRY RUN: would gpg --detach-sign $ARTIFACT"
  [[ "$MINISIGN" -eq 1 ]] && echo "DRY RUN: would minisign -Sm $ARTIFACT"
  exit 0
fi

if [[ "$GPG" -eq 1 ]]; then
  if [[ -z "$KEY" ]]; then
    echo "GPG signing requested but --key not provided; skipping GPG." >&2
  elif command -v gpg >/dev/null 2>&1; then
    gpg --detach-sign --armor --local-user "$KEY" "$ARTIFACT"
  else
    echo "gpg not found" >&2
    exit 1
  fi
fi

if [[ "$MINISIGN" -eq 1 ]]; then
  if [[ -z "$KEY" ]]; then
    echo "minisign requested but --key not provided; skipping minisign." >&2
  elif command -v minisign >/dev/null 2>&1; then
    minisign -S -s "$KEY" -m "$ARTIFACT"
  else
    echo "minisign not found" >&2
    exit 1
  fi
fi

echo "Linux signing hooks completed for $ARTIFACT"
