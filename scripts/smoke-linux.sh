#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ARTIFACT_DIR="${1:-$ROOT/dist}"

TARBALL="$(ls -1t "$ARTIFACT_DIR"/Zarya-*-linux-*.tar.gz 2>/dev/null | head -n 1 || true)"
if [[ -z "$TARBALL" ]]; then
  echo "No Linux tarball found in $ARTIFACT_DIR" >&2
  exit 1
fi

python3 "$ROOT/scripts/smoke-package.py" --artifact "$TARBALL"
