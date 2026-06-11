# Zarya Windows Installer PoC (WiX)

0.34 uses **WiX Toolset v4** as the Windows installer proof of concept.

Portable ZIP remains the **recommended beta distribution**.

## Requirements

- WiX Toolset 4.x (`wix` on PATH)
- Visual Studio 2022+ build of Zarya
- Qt deployment tools (`windeployqt`) for shared Qt builds
- Optional: code signing certificate for `.\scripts\package-windows-msi.ps1 -Sign`

## Layout

| Path | Role |
|------|------|
| `Product.wxs` | MSI package definition |
| `Directories.wxs` | Program Files + Start Menu roots |
| `Registry.wxs` | `HKLM\Software\Zarya` + `.zarya-installed` marker |
| `Shortcuts.wxs` | Start Menu + optional Desktop shortcut |
| `Variables.wxi` | Product metadata (stable `UpgradeCode`) |
| `generated/` | Build-time harvested components (not committed) |

## Build

```powershell
.\scripts\package-windows-msi.ps1 -Configuration Release -OutputDir .\dist
```

Artifact:

`Zarya-0.34.0-beta-windows-x64-installer-poc.msi`

## Install

```powershell
msiexec /i .\dist\Zarya-0.34.0-beta-windows-x64-installer-poc.msi
```

Optional properties:

```powershell
msiexec /i Zarya-....msi INSTALLDESKTOPSHORTCUT=1
msiexec /i Zarya-....msi INSTALLHELPERSERVICE=1
```

Defaults: no Desktop shortcut, **no helper service**.

## Uninstall

```powershell
msiexec /x .\dist\Zarya-0.34.0-beta-windows-x64-installer-poc.msi
```

User data under `%AppData%\Zarya` and `%LocalAppData%\Zarya` is **preserved**.

## Helper service

Experimental. Required only for TUN / kill switch scenarios. **Not installed by default.**

See [docs/installer/windows-msi-poc.md](../../../docs/installer/windows-msi-poc.md).
