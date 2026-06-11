# Known Limitations

## Recommended stable path

**Xray system-proxy mode** is the recommended path for beta and future stable releases.

sing-box TUN, zarya-helper, and kill switch are **experimental** and may be hidden when using the stable release channel. See [docs/stable/stable-scope.md](../stable/stable-scope.md).

## System proxy is not VPN mode

System proxy mode only affects applications that respect OS proxy settings.
It does not capture all traffic.

## TUN mode is experimental

TUN mode can change routes and DNS behavior.
It may require zarya-helper and elevated privileges.

**Do not use experimental TUN unless you understand route/DNS changes.**

## Kill switch is experimental

Linux nftables and Windows WFP kill switch implementations are PoCs.
Use recovery instructions if networking is blocked.

## macOS kill switch is not implemented

macOS kill switch is not available in this beta.

## No app self-update

Zarya can update Xray and sing-box cores, but it cannot update the Zarya app itself yet.

## Builds may be unsigned

Use SHA256 checksums to verify downloads.

## No production installer yet

Portable/bundle artifacts are the **primary beta distribution format**.

Windows has an **MSI PoC** (0.34) for testing installed layout — see [docs/installer/windows-msi-poc.md](../installer/windows-msi-poc.md). It is not a production-signed installer.
