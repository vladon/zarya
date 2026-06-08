# Zarya Core Update Manager

## Managed cores

- **Xray** — system proxy runtime
- **sing-box** — experimental TUN runtime

Zarya downloads upstream release archives from GitHub, verifies checksums when available, extracts to staging, and installs into the managed cores directory.

## Managed vs external paths

By default, only cores inside the Zarya-managed directory are updated:

- Portable: `cores/xray/`, `cores/sing-box/`
- Non-portable: under application data `cores/`

If Settings points to an external executable path, status shows **External** and update is blocked unless **Allow managing cores outside Zarya-managed directory** is enabled.

## Release sources

| Core | Provider |
|------|----------|
| Xray | GitHub `XTLS/Xray-core` latest release |
| sing-box | GitHub `SagerNet/sing-box` latest release |

No GitHub token is required. Rate limits may require retrying later.

## Checksum policy

When a companion checksum asset is found (`.sha256`, `SHA256SUMS`, etc.), the downloaded archive is verified before install.

If no checksum is available:

- Default: install is **blocked**
- Optional setting: allow install with explicit warning

## Install flow

1. Refuse if the relevant core is running
2. Select platform/arch asset from release
3. Download archive to `runtime/core-updates/downloads/`
4. Verify checksum (if available)
5. Extract to `runtime/core-updates/extract/`
6. Verify staged executable (`version` command)
7. Backup current core to `cores/.backup/`
8. Copy new executable; preserve Xray `geoip.dat` / `geosite.dat`
9. Write `VERSION` and `.zarya-core.json`
10. Final verification; rollback backup on failure

## Rollback

**Tools → Core Manager → Rollback** restores the latest backup for the selected core (when not running).

Backup retention is configurable in Settings → Core updates (default: 2).

## Portable mode

Managed cores live under `./cores/` next to the portable Zarya executable.

## Troubleshooting

- **Cannot update while running** — stop the profile / TUN session first
- **Checksum unavailable** — enable optional setting or install manually
- **GitHub rate limit** — wait and use Check Versions again
- **Extraction failed** — ensure `tar` is available on PATH (Windows 10+ includes tar)

## Non-goals (0.19)

- Zarya self-update
- Helper / Wintun driver update
- OS package managers (winget, homebrew, apt)
- Code signing / notarization validation
