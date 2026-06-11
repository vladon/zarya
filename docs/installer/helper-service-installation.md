# Helper Service Installation

## Helper is optional

**Required for:**

- TUN mode (recommended path)
- kill switch
- privileged route/firewall operations

**Not required for:**

- Xray system-proxy mode
- profile/subscription management
- backup/diagnostics
- core updates

## Windows

Windows Service:

- service name: `ZaryaHelper`
- start type: manual by default
- account: LocalSystem for PoC — review before production
- installed by MSI/installer in a future milestone

See `packaging/windows/wix/Service.wxs` skeleton.

## Linux

systemd + polkit:

- `zarya-helper.service`
- `dev.vladon.zarya.helper.policy`

Templates exist under `packaging/linux/`.

## macOS

SMAppService / LaunchDaemon direction. Production requires signing/notarization review.

See `docs/service/macos-smappservice.md`.

## Installer rule

Never auto-install helper without explicit user consent.
