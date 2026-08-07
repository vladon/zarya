# Zarya Core Update Manager

## Runtime distributions

- **Xray** — embedded system-proxy runtime. Core Manager shows **Built into Zarya**,
  the Xray/ABI versions and load status. Download, update, rollback, folder and path
  actions are disabled; Xray is updated with the app.
- **sing-box** — external managed executable for experimental TUN. Existing download,
  checksum, install and rollback behavior remains unchanged.

`cores/xray/` stores geo assets only. Managed executable paths and the update flow below
apply exclusively to sing-box. Existing `cores/xrayPath` values are inert and ignored.

## Checksum policy

When a companion checksum asset is found (`.sha256`, `.dgst`, `SHA256SUMS`, etc.), the downloaded archive is verified before install.

If no sidecar exists, Zarya uses the GitHub Releases API `digest` field (`sha256:…`) when present (sing-box and most modern GitHub assets).

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
8. Embedded sing-box is updated only through the Zarya app update; no core executable is downloaded.
9. Write `VERSION` and `.zarya-core.json`
10. Final verification; rollback backup on failure

**Update Selected / Update All** only download when the core is missing or the installed version differs from the latest checked release. Already up-to-date cores are skipped.

## Rollback

**Tools → Core Manager → Rollback** restores the latest sing-box backup when it is not running.

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
