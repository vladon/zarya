# Zarya 1.4.0

## Stable release

1.4.0 adds WireGuard profiles through Xray, silent startup recovery, and the new Desktop App Toolkit UI foundation.

- Import `wireguard://` / `wg://` links and run complete WireGuard profiles through Xray userspace mode
- Restore stale system-proxy/runtime state silently after an unclean shutdown
- System, light, and dark themes with a `lib_ui`-based configured status strip
- Desktop App Toolkit is mandatory in official builds; Windows releases use static Qt
- Continues shipping pinned Xray and runetfreedom geo data for offline first start

See [docs/release-notes/1.4.0.md](docs/release-notes/1.4.0.md) and [docs/stable/stable-scope.md](docs/stable/stable-scope.md).

## Recommended path

Xray system-proxy mode (stable scope).

## Reporting issues

**Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.
