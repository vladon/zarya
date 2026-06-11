# Zarya 0.32.0-beta

## App self-update design

0.32 adds update manifest checking, download-and-verify, and updater docs. **The app does not replace itself yet.**

Core Manager still updates Xray/sing-box. App updates update Zarya itself.

## What's new

- **Help → Check for App Updates…** — manifest check, asset selection, SHA-256 verify
- Settings → **App updates** (separate from Core updates)
- `docs/updater/` design docs and `scripts/generate-update-manifest.py`

See [docs/release-notes/0.32-beta.md](docs/release-notes/0.32-beta.md) and [docs/updater/README.md](docs/updater/README.md).

## Recommended mode

Xray system-proxy mode.

## Reporting issues

**Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.

See [docs/public-beta/reporting-issues.md](docs/public-beta/reporting-issues.md).
