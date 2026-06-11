# Zarya Stable Scope

## Stable in 1.0

The following features are candidates for stable support:

- Xray system-proxy runtime
- Profile management
- Subscription management
- VLESS / VMess / Trojan / Shadowsocks through Xray
- Routing profiles for Xray
- DNS profiles for Xray
- Xray Geo Data Manager
- Core Manager for Xray and sing-box
- Node testing
- Tray / safe shutdown
- Startup recovery
- Backup/import
- Diagnostics bundle
- English/Russian UI
- Portable packaging

## Experimental / not stable

- sing-box TUN mode
- zarya-helper
- Linux nftables kill switch
- Windows WFP kill switch
- helper service installation
- app self-update installation
- production installer
- macOS kill switch

## Default recommendation

Use **Xray system-proxy mode**.

Production TUN is **not** required for 1.0. Experimental networking features remain available on beta/dev channels with explicit opt-in. **1.0.0 stable** builds hide experimental features by default (same policy as RC).
