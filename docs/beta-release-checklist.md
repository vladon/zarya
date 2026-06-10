# Zarya Beta Release Checklist (0.26)

Quick gate before publishing `0.26.0-beta`. For full manual coverage see [beta-regression-checklist.md](beta-regression-checklist.md).

## Automated smoke

```powershell
cmake --build build --config Release --target zarya zarya-helper zarya_smoke_test
python scripts/run-smoke-tests.py --source-tree . --build-dir build
```

With release artifact:

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist
python scripts\run-smoke-tests.py --artifact .\dist\Zarya-0.26.0-beta-windows-x64-portable.zip --build-dir .\build
```

## Build

- [ ] Windows portable ZIP builds (`scripts/package-windows.ps1`)
- [ ] macOS app bundle builds (`scripts/package-macos.sh`)
- [ ] Linux tarball builds (`scripts/package-linux.sh`)
- [ ] `run-smoke-tests.py` passes
- [ ] SHA256 sidecars generated

## First run

- [ ] Starts with empty config
- [ ] First-run wizard appears
- [ ] Tray works
- [ ] No crash without cores
- [ ] Choose existing binary opens file dialog

## Core manager

- [ ] Installs Xray
- [ ] Installs sing-box
- [ ] Blocks update while running
- [ ] Rollback works when backup exists

## Basic proxy mode

- [ ] Import VLESS link
- [ ] Start Xray
- [ ] System proxy enabled
- [ ] Stop restores proxy
- [ ] Kill app → startup recovery restores proxy

## Subscriptions

- [ ] Add subscription (wizard + manager)
- [ ] Update subscription
- [ ] Failed update keeps old profiles

## Routing/DNS

- [ ] Proxy All / Bypass LAN validate
- [ ] Secure Remote DNS generates dns object

## Backup/diagnostics

- [ ] Export full + redacted backup
- [ ] Import preview + pre-import backup
- [ ] Diagnostics bundle redacted (no raw share links / helper.token)

## Shutdown/recovery

- [ ] Close to tray
- [ ] Tray exit safe shutdown
- [ ] Unclean shutdown recovery dialog

## Experimental (optional)

- [ ] TUN disabled by default, warning shown
- [ ] Kill switch recovery documented and tested on target OS
