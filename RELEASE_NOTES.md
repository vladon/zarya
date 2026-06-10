# Zarya 0.27.0 Beta

## Beta status

This is a beta-quality build with signing-ready packaging hooks. Artifacts are **unsigned by default** unless you explicitly enable signing with credentials.

Use SHA256 checksums from the release page to verify downloads. See [docs/signing/README.md](docs/signing/README.md).

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

- Signed builds are optional in 0.27; notarization is not a release gate
- No app self-update
- macOS kill switch unsupported
- TUN mode is experimental
- Xray and sing-box are not bundled in release artifacts
- No production service installer

## Reporting issues

See [docs/beta-bug-triage.md](docs/beta-bug-triage.md).
