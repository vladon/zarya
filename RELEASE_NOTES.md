# Zarya 0.26.0 Beta

## Beta status

This is the first beta-quality build. Use Xray system-proxy mode for the stable path.

## Recommended path

1. Install Xray via Core Manager (or choose an existing binary)
2. Import a profile or subscription
3. Start with system proxy enabled
4. Stop and verify proxy restore before testing experimental features

## Stable enough for testing

- Xray system proxy
- Profiles and subscriptions
- Routing and DNS profiles
- Core Manager
- Backup import/export
- Diagnostics bundle (strict redaction)
- English and Russian UI

## Experimental

- sing-box TUN mode
- zarya-helper
- Linux nftables kill switch PoC
- Windows WFP kill switch PoC

## Known limitations

- No signed builds or notarization
- No app self-update
- macOS kill switch unsupported
- TUN mode is experimental
- Xray and sing-box are not bundled in release artifacts
- No production service installer

## Reporting issues

Use **Help → Create Diagnostics Bundle** and attach the archive with your report.

See [docs/beta-bug-triage.md](docs/beta-bug-triage.md) and [docs/beta-regression-checklist.md](docs/beta-regression-checklist.md).
