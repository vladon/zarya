# Zarya 0.29.0 Beta

This is the first public beta preparation build.

## Recommended mode

Use **Xray system-proxy mode**.

## Downloads

- Windows x64 portable ZIP
- macOS Apple Silicon ZIP
- macOS Intel ZIP (if built)
- Linux x64 tar.gz

## Verify downloads

Use SHA256 checksums from `SHA256SUMS.txt`.

## What works

- Xray system-proxy runtime
- Profile and subscription management
- Routing/DNS profiles
- Core Manager
- Backup/import
- Diagnostics bundle
- English/Russian UI

## Experimental

- sing-box TUN mode
- zarya-helper
- Linux nftables kill switch PoC
- Windows WFP kill switch PoC

**Do not use experimental TUN unless you understand route/DNS changes.**

## Known limitations

- builds may be unsigned
- macOS kill switch unsupported
- no app self-update
- no production installer yet

## Reporting bugs

Create a diagnostics bundle: **Help → Create Diagnostics Bundle**

Do not post raw proxy links or subscription URLs.

See `docs/public-beta/reporting-issues.md` in the repository for the full workflow.
