# Update Recovery

## If update fails

Zarya attempts rollback automatically when the post-update version check fails.

Check:

- `runtime/app-updates/update-failed.json`
- `runtime/app-updates/last-update.log`
- `runtime/app-updates/logs/`

If rollback succeeds, Zarya restarts and shows a rollback notice.

## Manual recovery

1. Close Zarya and ensure no `Zarya.exe` / `zarya` process is running.
2. Open `runtime/app-updates/backups/`.
3. Choose the most recent `pre-<version>-<timestamp>/` folder.
4. Copy the backed-up app files back into the Zarya app directory.
5. Preserve existing `data/`, `runtime/`, `portable.flag`, and `cores/`.

## Critical failure

If rollback fails, `runtime/app-updates/update-critical-failure.txt` is written.

In that case:

1. Do not delete `data/` or `cores/`.
2. Restore app binaries manually from the newest backup folder.
3. If the app still cannot start, use **Help → Create Diagnostics Bundle** from a working copy or reinstall portable Zarya and import backup data.

## Installed mode

Automatic installed-mode updates are not implemented. Use the installer manually and preserve user data under `%AppData%` / `%LocalAppData%`.
