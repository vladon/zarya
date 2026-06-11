# Zarya App Updater

## Current state

Zarya 0.32 can check update manifests and download/verify artifacts.
It does **not** replace the running app.

Core Manager updates Xray and sing-box. The app updater updates **Zarya itself**. These are separate mechanisms.

## Goals

- Safe update discovery
- Checksum/signature verification design
- Manual install path for beta
- Future automatic update support

## Non-goals (0.32)

- Silent updates
- Unsigned automatic install
- Helper service update in the same step
- Delta updates
- Mandatory updates

## Related docs

- [update-manifest.md](update-manifest.md)
- [portable-update-flow.md](portable-update-flow.md)
- [installed-update-flow.md](installed-update-flow.md)
- [updater-security.md](updater-security.md)
- [helper-update.md](helper-update.md)
