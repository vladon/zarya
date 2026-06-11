# Uninstall and Repair

## Uninstall (default)

- remove application files
- stop helper service if installed
- remove helper service registration
- **do not** delete user data by default
- **do not** delete backups by default
- **do not** delete downloaded cores by default unless installed under app dir

## Optional “remove all data”

Only with explicit confirmation. May delete:

- profiles, subscriptions, routing/DNS
- backups, diagnostics exports
- downloaded cores in user cache

## Repair

- verify application files (checksum/manifest)
- restore missing translations/docs
- reinstall helper service if user previously selected it
- run helper service self-test
- **do not** overwrite user config

## Recovery

- remove stale helper service
- recover kill switch rules (platform-specific)
- restore system proxy if Zarya-owned state is detected

Repair and recovery reuse existing startup recovery and helper recovery docs where possible.
