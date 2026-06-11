# Zarya Installer Strategy

## Current state

Zarya currently ships primarily as portable/bundle artifacts (ZIP, tarball, `.app` in archive).

## Target state

- **Windows:** installer with optional helper service installation
- **macOS:** signed/notarized `.app` distributed through DMG; PKG later if privileged helper is required
- **Linux:** AppImage/tarball first, then deb/rpm packages

## Principles

- Portable mode remains supported.
- Installing the privileged helper is optional.
- Xray system-proxy mode must work without helper.
- TUN/kill switch may require helper.
- Uninstall must not destroy user data by default.
- Repair/recovery must remove stale helper/service/firewall state.

## Documents

| Doc | Purpose |
|-----|---------|
| [installed-layout.md](installed-layout.md) | Target on-disk layout per platform |
| [windows-installer-strategy.md](windows-installer-strategy.md) | WiX/MSI direction |
| [windows-msi-poc.md](windows-msi-poc.md) | **0.34** Windows MSI PoC (build/install) |
| [macos-installer-strategy.md](macos-installer-strategy.md) | DMG/PKG direction |
| [linux-packaging-strategy.md](linux-packaging-strategy.md) | AppImage/deb/rpm |
| [portable-to-installed-migration.md](portable-to-installed-migration.md) | Explicit migration flow |
| [uninstall-repair.md](uninstall-repair.md) | Uninstall, repair, recovery |
| [helper-service-installation.md](helper-service-installation.md) | Optional helper install |
| [installer-security.md](installer-security.md) | Threat model and signing |

**0.31–0.33:** planning and stable scope.

**0.34:** Windows WiX MSI **PoC** (`scripts/package-windows-msi.ps1`). Portable ZIP remains recommended. No production-signed MSI yet.
