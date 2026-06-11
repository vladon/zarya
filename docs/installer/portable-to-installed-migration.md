# Portable to Installed Migration

## Problem

Portable mode stores data next to the executable (`data/`, `portable.flag`).
Installed mode stores data in OS-specific user directories.

## Migration must be explicit

Zarya must **not** silently move user data.

## Flow

1. Installed Zarya detects portable data folder selected by user.
2. Shows preview: profiles, subscriptions, routing, DNS, settings, backups.
3. Creates backup of installed state (pre-import backup).
4. Imports portable data using the existing backup import mechanism.
5. Leaves the original portable folder unchanged.

## UI (0.31)

**File → Import from Portable Zarya Folder…**

1. User selects portable root folder.
2. Zarya checks for `portable.flag` or `data/` with config files.
3. Shows preview counts.
4. Creates a temporary `.zarya-backup.zip` from portable data.
5. Opens the standard Import Backup dialog.

No automatic deletion of the portable folder.

## Future installer integration

When a production installer is available, the installer may offer migration during first launch. That flow will reuse the same preview + backup import path.
