# Experimental Features

These features are **not** part of the stable 1.0 scope. On the stable release channel they are hidden unless you explicitly enable **Show experimental features** in Settings.

## sing-box TUN mode

Use only if you understand route/DNS changes.

Requirements:

- sing-box core
- zarya-helper recommended
- elevated privileges may be required

## zarya-helper

The helper is used for privileged operations such as TUN and kill switch management.

## Kill switch

Kill switch is experimental.

**Linux:** nftables PoC

**Windows:** WFP PoC

**macOS:** unsupported

## Recommendation

For normal beta testing, use **Xray system-proxy mode**.
