# Zarya Stable Scope

## Stable in the current release

The following features are supported on the stable channel:

- Xray system-proxy runtime on Windows, macOS, Linux GNOME, and Linux KDE/Plasma when the
  platform's native proxy facility is available
- Profile management
- Subscription management
- VLESS / VMess / Trojan / Shadowsocks / Hysteria2 / WireGuard through Xray
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

Production TUN is not part of the stable scope. Experimental networking features remain available on beta/dev channels with explicit opt-in. Stable builds hide experimental features by default.

System-proxy mode is not a VPN. It affects only applications that honor WinINet,
`networksetup`, GNOME `gsettings`, or KDE/KIO proxy settings on the selected platform.
