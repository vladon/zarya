# Zarya 1.2.0

## Stable release

1.2.0 ships a bundled Xray seed for offline first start, simplifies local proxy to a single mixed port, and adds Test URL presets.

- Bundled pinned Xray (v26.3.27) in release artifacts under `cores/xray/` — no download required on first launch; Core Manager and external paths still work
- Single local mixed SOCKS5/HTTP inbound (`mixedPort`, default `10808`) instead of separate SOCKS/HTTP ports
- Test URL presets with Cloudflare as the default latency-test URL

See [docs/release-notes/1.2.0.md](docs/release-notes/1.2.0.md) and [docs/stable/stable-scope.md](docs/stable/stable-scope.md).

## Recommended path

Xray system-proxy mode (stable scope).

## Reporting issues

**Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.
