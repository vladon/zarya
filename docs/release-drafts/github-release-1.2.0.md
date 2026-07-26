# Zarya 1.2.0

Stable release with a bundled Xray seed, a single mixed local proxy port, and Test URL presets.

## Highlights

- Bundled pinned Xray (`v26.3.27`) in release artifacts for offline first start; Core Manager and external paths still work
- Single local mixed SOCKS5/HTTP proxy port (default `10808`)
- Test URL presets (Cloudflare default)

## Recommended mode

Use Xray system-proxy mode.

## Downloads

- Windows x64 portable ZIP
- macOS arm64 ZIP
- Linux x64 tar.gz

## Verify downloads

Use `SHA256SUMS.txt`.

## Experimental features

TUN/helper/kill switch remain experimental and are disabled by default.

## Known limitations

System proxy mode is not VPN mode. Some applications may ignore OS proxy settings.
Geo data is not bundled; use Geo Data Manager when bypass routing needs it.

## Issue reports

Use **Help → Create Diagnostics Bundle**.
Do not post raw proxy links or subscription URLs.
