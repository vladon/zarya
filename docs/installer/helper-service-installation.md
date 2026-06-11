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
- optional MSI install when `INSTALLHELPERSERVICE=1` (0.34 PoC; off by default)

See `packaging/windows/wix/README.md` and `scripts/generate-wix-components.py` (optional `ServiceInstall` on `zarya-helper.exe`).

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
