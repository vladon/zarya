# Zarya Beta Regression Checklist (0.26)

Run before tagging `0.26.0-beta`. Use `python scripts/run-smoke-tests.py` for automated checks.

## Fresh start

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Extract clean portable artifact | [ ] | [ ] | [ ] | | |
| First-run wizard appears | [ ] | [ ] | [ ] | | |
| No crash without cores | [ ] | [ ] | [ ] | | |
| `data/` and `runtime/` created | [ ] | [ ] | [ ] | | |
| Language default applied | [ ] | [ ] | [ ] | | |

## Core Manager

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Opens with missing cores | [ ] | [ ] | [ ] | | |
| Check Versions works | [ ] | [ ] | [ ] | | |
| Install Xray | [ ] | [ ] | [ ] | | |
| Installed version detected | [ ] | [ ] | [ ] | | |
| Install sing-box | [ ] | [ ] | [ ] | | |
| Rollback only when backup exists | [ ] | [ ] | [ ] | | |
| Choose existing binary (file dialog) | [ ] | [ ] | [ ] | | |

## Profile import

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Paste `vless://` link | [ ] | [ ] | [ ] | | |
| Parsed count correct | [ ] | [ ] | [ ] | | |
| Import creates profile | [ ] | [ ] | [ ] | | |
| Start enabled after import | [ ] | [ ] | [ ] | | |

## Subscription update

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Add HTTP subscription | [ ] | [ ] | [ ] | | |
| Update subscription | [ ] | [ ] | [ ] | | |
| Counts correct (added/updated/missing) | [ ] | [ ] | [ ] | | |
| Manual profiles untouched | [ ] | [ ] | [ ] | | |
| Failed update keeps old profiles | [ ] | [ ] | [ ] | | |

## Xray runtime

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Start valid profile | [ ] | [ ] | [ ] | | |
| `xray run -test` passes | [ ] | [ ] | [ ] | | |
| Local HTTP/SOCKS ports up | [ ] | [ ] | [ ] | | |
| Stop terminates process | [ ] | [ ] | [ ] | | |
| Logs do not expose credentials | [ ] | [ ] | [ ] | | |

## System proxy

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Start → proxy applied | [ ] | [ ] | [ ] | | |
| Stop → previous state restored | [ ] | [ ] | [ ] | | |
| Tray exit → proxy restored | [ ] | [ ] | [ ] | | |
| Kill app → restart recovery | [ ] | [ ] | [ ] | | |

## Routing / DNS / Geo

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Proxy All validates | [ ] | [ ] | [ ] | | |
| Bypass LAN validates | [ ] | [ ] | [ ] | | |
| Bypass RU warns without geo files | [ ] | [ ] | [ ] | | |
| Geo Data Manager opens from warning | [ ] | [ ] | [ ] | | |
| Secure Remote DNS generates dns object | [ ] | [ ] | [ ] | | |
| System DNS omits dns object (Xray) | [ ] | [ ] | [ ] | | |

## Node testing

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| TCP test selected | [ ] | [ ] | [ ] | | |
| Real delay test | [ ] | [ ] | [ ] | | |
| Test all with concurrency | [ ] | [ ] | [ ] | | |
| Cancel tests | [ ] | [ ] | [ ] | | |
| Main Xray keeps running | [ ] | [ ] | [ ] | | |

## Tray / safe shutdown

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Close hides to tray | [ ] | [ ] | [ ] | | |
| Tray restore | [ ] | [ ] | [ ] | | |
| Tray Stop | [ ] | [ ] | [ ] | | |
| Tray Exit safe shutdown | [ ] | [ ] | [ ] | | |
| File → Exit same as tray | [ ] | [ ] | [ ] | | |

## Startup recovery

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Unclean shutdown detected | [ ] | [ ] | [ ] | | |
| Restore system proxy option | [ ] | [ ] | [ ] | | |
| Recover kill switch option | [ ] | [ ] | [ ] | | |
| Clean runtime temp files | [ ] | [ ] | [ ] | | |

## Backup / import

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Export full backup | [ ] | [ ] | [ ] | | |
| Export redacted backup | [ ] | [ ] | [ ] | | |
| Import preview | [ ] | [ ] | [ ] | | |
| Pre-import backup created | [ ] | [ ] | [ ] | | |
| Import blocked while running | [ ] | [ ] | [ ] | | |

## Diagnostics

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Create diagnostics bundle | [ ] | [ ] | [ ] | | |
| Strict redaction (no share links) | [ ] | [ ] | [ ] | | |
| `helper.token` not included | [ ] | [ ] | [ ] | | |
| Manifest language-neutral | [ ] | [ ] | [ ] | | |

## Localization

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| System default language | [ ] | [ ] | [ ] | | |
| English UI | [ ] | [ ] | [ ] | | |
| Russian UI | [ ] | [ ] | [ ] | | |
| Restart after language change | [ ] | [ ] | [ ] | | |
| Russian plurals | [ ] | [ ] | [ ] | | |

## Packaging

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Package script succeeds | [ ] | [ ] | [ ] | | |
| `run-smoke-tests.py` passes | [ ] | [ ] | [ ] | | |
| SHA256 sidecar matches | [ ] | [ ] | [ ] | | |
| No forbidden files in artifact | [ ] | [ ] | [ ] | | |

## TUN experimental

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Disabled by default | [ ] | [ ] | [ ] | | |
| Warning on enable | [ ] | [ ] | [ ] | | |
| Missing sing-box → actionable error | [ ] | [ ] | [ ] | | |
| Helper unavailable → actionable error | [ ] | [ ] | [ ] | | |
| Safe exit stops TUN | [ ] | [ ] | [ ] | | |

## Helper

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Helper starts with token auth | [ ] | [ ] | [ ] | | |
| Invalid token rejected | [ ] | [ ] | [ ] | | |
| Path outside allowed dirs rejected | [ ] | [ ] | [ ] | | |
| External sing-box path allowed when configured | [ ] | [ ] | [ ] | | |

## Kill switch experimental

| Test | Windows | macOS | Linux | Result | Notes |
|------|---------|-------|-------|--------|-------|
| Default off | [ ] | [ ] | [ ] | | |
| Linux enable/disable PoC | [ ] | n/a | [ ] | | |
| Windows WFP PoC (admin) | [ ] | n/a | n/a | | |
| Recovery after stale marker | [ ] | [ ] | [ ] | | |
