# Zarya App Updater

## Current state (0.35)

Zarya can:

- check update manifests
- download and verify portable/installer artifacts
- **install portable updates on Windows/Linux** using external `zarya-updater`
- roll back automatically if post-update verification fails

Installed MSI mode and macOS `.app` bundles support check/download/verify only in 0.35.

Core Manager updates Xray and sing-box. The app updater updates **Zarya itself**. These are separate mechanisms.

## Portable update PoC

See [portable-update-implementation.md](portable-update-implementation.md).

Flow:

1. Download and verify SHA256
2. Stage archive under `runtime/app-updates/staging/`
3. Launch `zarya-updater --plan runtime/app-updates/pending-update.json`
4. Preserve `data/`, `runtime/`, `portable.flag`, `cores/`

## Non-goals (0.35)

- Silent updates
- Unsigned automatic install in beta/stable UI
- Helper service update in the same step
- Delta updates
- Mandatory updates
- Installed MSI self-update
- macOS `.app` replacement
- AppImage replacement

## Related docs

- [update-manifest.md](update-manifest.md)
- [portable-update-flow.md](portable-update-flow.md)
- [portable-update-implementation.md](portable-update-implementation.md)
- [recovery.md](recovery.md)
- [installed-update-flow.md](installed-update-flow.md)
- [updater-security.md](updater-security.md)
- [helper-update.md](helper-update.md)
