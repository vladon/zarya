# Portable App Update PoC

## Supported

- Windows portable ZIP update
- SHA256 verification
- staging
- external updater process (`zarya-updater`)
- rollback on failed post-update check
- Linux portable tarball (best-effort, when not running as AppImage)

## Stable policy (1.0.0)

- Update check and download/verify are available in stable builds.
- **Install is disabled by default** via `app/enablePortableUpdaterPoC` (default `false` for stable/rc).
- Dev and beta builds default install to enabled; stable users can download and verify manually.

## Not supported

- installed MSI update
- helper service update
- macOS `.app` replacement
- AppImage replacement
- silent update
- mandatory update
- delta update
- production auto-update

## Flow

1. Zarya checks the update manifest and downloads the portable artifact.
2. SHA256 is verified and the archive is extracted to `runtime/app-updates/staging/`.
3. Zarya writes `runtime/app-updates/pending-update.json`.
4. Zarya launches `zarya-updater --plan …` and exits.
5. `zarya-updater` waits for Zarya to exit, backs up app files, copies staged files, and runs `Zarya.exe --version`.
6. On success, Zarya restarts with `--post-update`.
7. On failure, rollback restores the backup and Zarya restarts with `--update-rollback`.

## Preserved paths

These paths are never replaced by the updater:

- `data/`
- `runtime/`
- `portable.flag`
- `cores/`

If an update archive contains `data/` or `runtime/` entries, they are skipped with a warning.

## Update state files

Under `runtime/app-updates/`:

- `pending-update.json`
- `update-success.json`
- `update-failed.json`
- `last-update.log`
- `logs/update-YYYYMMDD-HHMMSS.log`

## Related docs

- [portable-update-flow.md](portable-update-flow.md)
- [recovery.md](recovery.md)
- [updater-security.md](updater-security.md)
