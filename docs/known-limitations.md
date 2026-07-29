# Known Limitations

## System proxy mode is not VPN mode

Zarya system-proxy mode configures the OS to send application traffic through a local mixed SOCKS/HTTP proxy. It does not create a VPN tunnel and does not route all traffic automatically for every application.

Supported backends are Windows WinINet, macOS `networksetup`, GNOME `gsettings`, and KDE/Plasma
`kioslaverc` through KConfig 6 or 5. A backend is unavailable when its native tools or desktop
session service are missing. CLI applications may still require `HTTP_PROXY` / `HTTPS_PROXY`.

## TUN mode is experimental

sing-box TUN mode requires elevated privileges or the privileged helper. Route/DNS recovery after crashes is limited. Use only for testing.

## Kill switch is experimental

Linux nftables and Windows WFP implementations are proof-of-concept. They can block network access incorrectly after crashes. Recovery procedures are documented in [recovery.md](recovery.md).

## macOS kill switch unsupported

macOS does not install PF firewall rules in current milestones.

## Rule-set compatibility

sing-box rule sets depend on the installed sing-box version and local `.srs` files. Missing rule sets block TUN start when strict mode is enabled.

## App self-update (portable PoC)

0.35 adds portable-mode update via external `zarya-updater` after SHA256 verification.

Not available yet:

- installed MSI auto-update
- macOS `.app` replacement
- AppImage replacement
- silent/background update

See [updater/portable-update-implementation.md](updater/portable-update-implementation.md).

## No signed installer yet

Distribution is portable ZIP/tarball/app bundle skeleton. Code signing and a production installer service are not implemented.
