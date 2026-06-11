# Installed update flow

Installed mode should use platform package/installer semantics:

- **Windows** — MSI/installer update (WiX path)
- **macOS** — signed/notarized `.app` replacement or PKG
- **Linux** — package manager (deb/rpm) or AppImage replacement

Zarya must not bypass OS installer semantics for installed deployments.

0.32 shows a warning when only portable artifacts are available but `InstallationMode` is Installed. Automatic installation is disabled.
