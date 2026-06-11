# Zarya 0.35.0-beta

## Portable app updater PoC

0.35 adds a **portable-mode app update PoC** using external `zarya-updater`.

- Windows portable ZIP: download → SHA256 verify → stage → install → restart
- Preserves `data/`, `runtime/`, `portable.flag`, `cores/`
- Automatic rollback if post-update check fails
- Installed MSI mode: check/download/verify only (no auto-install yet)

See [docs/release-notes/0.35-beta.md](docs/release-notes/0.35-beta.md) and [docs/updater/portable-update-implementation.md](docs/updater/portable-update-implementation.md).

## Recommended path

Xray system-proxy mode (stable scope).

## Reporting issues

**Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.
