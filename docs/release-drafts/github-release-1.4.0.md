# Zarya 1.4.0

Stable release with WireGuard profiles through Xray, silent startup recovery, and the Desktop App Toolkit UI foundation.

## Highlights

- Import `wireguard://` / `wg://` links and run complete WireGuard profiles through Xray userspace mode
- Recover stale system-proxy and runtime state silently after an unclean shutdown
- System, light, and dark themes with a `lib_ui`-based configured status strip
- Mandatory Desktop App Toolkit integration on every platform
- Static Qt 6.8.3 Windows binaries with the static MSVC runtime
- Pinned Xray and runetfreedom geo data retained for offline first start

## Recommended mode

Use Xray system-proxy mode.

## Downloads

- Windows x64 portable ZIP
- macOS arm64 ZIP
- Linux x64 tar.gz

Use `SHA256SUMS.txt` to verify downloads.

## Licensing

Zarya-authored source is MIT or GPLv3+. Official binaries link Desktop App Toolkit and are GPLv3+.

## Experimental features

TUN, helper, kill switch, MSI, and app-update installation remain experimental and are disabled or hidden by default.

## Known limitations

System-proxy mode is not VPN mode. Some applications may ignore operating-system proxy settings.

## Issue reports

Use **Help → Create Diagnostics Bundle**. Do not post raw proxy links, private keys, or subscription URLs.
