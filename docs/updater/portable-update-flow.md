# Portable update flow

Future production flow (not implemented in 0.32):

1. Download new portable ZIP.
2. Verify checksum/signature.
3. Ensure runtime is stopped.
4. Create backup of current app directory or app binaries.
5. Extract to staging.
6. Start new version with `--post-update-check`.
7. If OK, replace old files.
8. Preserve `data/` and `portable.flag`.
9. Roll back if post-update check fails.

## Risks

- Replacing running executable on Windows
- Antivirus locking files
- Preserving user data

## External updater

For future implementation, use an external updater process (`zarya-updater`) so the running GUI is not responsible for replacing its own binary.

0.32 provides download-and-verify only via **Help → Check for App Updates…**.
