# Windows MSI Installer PoC

## Status

This is a **proof of concept** installer for milestone 0.34.

**Portable ZIP remains the recommended RC and beta distribution.**

0.34 uses WiX Toolset v4. It is not a production-signed MSI release. In 0.36 RC, MSI is published only as an experimental installer-poc artifact.

## What it installs

Per-machine under `C:\Program Files\Zarya\`:

- `Zarya.exe`
- `zarya-helper.exe`
- `translations\`
- `docs\`
- `cores\xray\`, `cores\sing-box\` placeholders
- Legal files and `release-manifest.json`
- Qt runtime files (shared builds via `windeployqt`)

Registry: `HKLM\Software\Zarya` (`InstallDir`, `Version`, `InstallerType`)

## What it does not install by default

- Xray / sing-box cores
- Helper **service** (binary is installed; service is optional)
- Wintun driver
- Geo data / rule sets

## User data

Installed mode stores data in:

- `%AppData%\Zarya\` — profiles, subscriptions, settings, backups
- `%LocalAppData%\Zarya\` — runtime, logs, core-updates

**Uninstall preserves user data.** To remove all data, delete those folders manually.

## Build

```powershell
.\scripts\package-windows-msi.ps1 -Configuration Release -OutputDir .\dist
```

Requires WiX 4.x on PATH (`wix build`).

Also build portable ZIP:

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist -SkipSigning
```

## Install

```powershell
msiexec /i .\dist\Zarya-0.34.0-beta-windows-x64-installer-poc.msi
```

Optional:

```powershell
msiexec /i Zarya-....msi INSTALLDESKTOPSHORTCUT=1
msiexec /i Zarya-....msi INSTALLHELPERSERVICE=1
```

## Uninstall

```powershell
msiexec /x .\dist\Zarya-0.34.0-beta-windows-x64-installer-poc.msi
```

## Helper service

Experimental. Only needed for TUN / kill switch. **Not installed by default.**

```powershell
sc.exe query ZaryaHelper
```

Should report service does not exist after a default install.

## Portable migration

The MSI does **not** auto-migrate portable data.

After first install:

- **File → Import from Portable Zarya Folder…**, or
- Export backup from portable Zarya → import in installed Zarya

## Smoke test

```powershell
.\scripts\smoke-windows-msi.ps1 -DistDir .\dist
.\scripts\smoke-windows-msi.ps1 -RequireMsi -DistDir .\dist
.\scripts\smoke-windows-msi.ps1 -Manual
```

Skeleton checks pass without a built MSI. Use `-RequireMsi` after `package-windows-msi.ps1`.
