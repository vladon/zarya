# Zarya 0.34.0-beta

## Windows installer PoC

0.34 adds a **WiX MSI proof of concept** for Windows. Portable ZIP remains the recommended beta distribution.

## What's new

- `scripts/package-windows-msi.ps1` — build `Zarya-0.34.0-beta-windows-x64-installer-poc.msi`
- Installed mode under `C:\Program Files\Zarya\` with Start Menu shortcut
- User data preserved on uninstall (`%AppData%\Zarya`, `%LocalAppData%\Zarya`)
- Optional helper service via `INSTALLHELPERSERVICE=1` (off by default)

See [docs/release-notes/0.34-beta.md](docs/release-notes/0.34-beta.md) and [docs/installer/windows-msi-poc.md](docs/installer/windows-msi-poc.md).

## Recommended path

Xray system-proxy mode (stable scope).

## Reporting issues

**Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.
