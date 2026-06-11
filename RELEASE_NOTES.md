# Zarya 0.28.0 Beta

## Beta status

0.28 adds helper service installation design. System-proxy mode does **not** require the helper service.

## Helper service

- Windows: optional `ZaryaHelper` service PoC
- Linux: systemd/polkit skeleton (manual install)
- macOS: SMAppService design only

See [docs/service/README.md](docs/service/README.md).

## Recommended path

1. Install Xray via Core Manager
2. Import a profile or subscription
3. Start with system proxy enabled
4. Use experimental TUN/helper only when needed

## Reporting issues

See [docs/beta-bug-triage.md](docs/beta-bug-triage.md).
