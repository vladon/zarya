# Zarya Backup Import/Export

## Backup archive format

Zarya backups use a single ZIP archive with extension `.zarya-backup.zip`.

Layout:

- `manifest.json` — format metadata, categories, SHA-256 checksums
- `data/` — JSON and settings (profiles, subscriptions, routing, DNS, settings.ini, etc.)
- `optional/` — large optional artifacts (rule-set `.srs`, Xray geo files)
- `diagnostics/` — app info, paths, redaction report (diagnostic backups)

Format version: **1** (`format: zarya-backup`).

## Categories

| Category | Default export | Notes |
|----------|----------------|-------|
| Profiles | yes | `data/profiles.json` |
| Subscriptions | yes | `data/subscriptions.json` |
| Routing profiles | yes | `data/routing.json` |
| DNS profiles | yes | `data/dns.json` |
| App settings | yes | `data/settings.ini` |
| Geo data settings | yes | `data/geodata-settings.json` |
| Rule-set metadata | yes | `data/rulesets.json` |
| Core metadata | yes | `data/core-metadata.json` |
| Rule-set `.srs` files | no | `optional/rule-set/` |
| Xray geo files | no | `optional/geo/` |

Core binaries are **not** included by default (use Core Manager to reinstall).

## Full backup

**File → Export Backup… → Full configuration backup**

Exports selected categories without redaction. Checksums are recorded in `manifest.json`.

## Redacted diagnostic backup

**File → Export Backup… → Redacted diagnostic backup**

Strict redaction removes credentials, hostnames, subscription URLs, and sensitive settings paths.
Includes `diagnostics/redaction-report.json`.

Use for support tickets — not for restoring a working configuration.

## Import modes

Per category on import:

- **Merge** (default for profiles/subscriptions/routing/DNS) — update by id, add new items
- **Replace** — replace stored data (built-in routing/DNS profiles preserved)
- **Skip** (default for settings) — do not import

## Pre-import backup

Before any import, Zarya creates:

`data/backups/pre-import-backup-YYYYMMDD-HHMMSS.zarya-backup.zip`

Import is **blocked** if pre-import backup fails.

## What is not included

Never exported:

- `runtime/` generated configs (may contain secrets)
- `helper.token`
- Kill switch recovery marker
- Core binaries (by default)
- Logs (unless explicitly added in a future version)

## Machine-specific settings

By default, import **skips**:

- Core executable paths
- Autostart / login item state
- Window geometry
- Last started profile id
- macOS preferred network service
- Runtime session flags

Enable **Import machine-specific settings** only when restoring on the same machine.

## Security notes

- Backups are not encrypted.
- Full backups contain credentials and subscription URLs — store securely.
- Diagnostic backups redact secrets but may still reveal structure and counts.

## Troubleshooting

| Issue | Action |
|-------|--------|
| Checksum mismatch | Archive corrupted or modified; re-export |
| Unsupported format version | Update Zarya or use matching version |
| Import blocked, core running | Stop Xray / sing-box TUN first |
| Import blocked, kill switch | Disable kill switch and recover if needed |
| Redacted backup import | Credentials cannot be restored; use full backup |
