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

## App self-update (portable PoC)

0.35 adds a **portable-mode update PoC** using external `zarya-updater`.

Supported in 0.35:

- Windows portable ZIP in-place update after SHA256 verification
- Linux portable tarball (best-effort; not AppImage)

Not supported in 0.35:

- installed MSI auto-update
- macOS `.app` replacement
- AppImage replacement
- silent/background update

See [docs/updater/portable-update-implementation.md](../updater/portable-update-implementation.md).

## Builds may be unsigned

Use SHA256 checksums to verify downloads.

## No production installer yet

Portable/bundle artifacts are the **primary beta distribution format**.

Windows has an **MSI PoC** (0.34) for testing installed layout — see [docs/installer/windows-msi-poc.md](../installer/windows-msi-poc.md). It is not a production-signed installer.
