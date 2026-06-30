# Known Limitations

## Recommended stable path

**Zarya 1.0.0 stable path is Xray system-proxy mode.**

sing-box TUN, zarya-helper, and kill switch are **experimental** and are hidden/disabled by default in stable builds. See [docs/stable/stable-scope.md](../stable/stable-scope.md).

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

macOS kill switch is not available in stable 1.0.0.

## App self-update (portable PoC)

Zarya includes a **portable-mode update PoC** using external `zarya-updater`.

On **1.0.0 stable**, update check and download/verify are available; **install is disabled by default**.

Supported when install is explicitly enabled (dev/beta channels by default):

- Windows portable ZIP in-place update after SHA256 verification
- Linux portable tarball (best-effort; not AppImage)

Not supported:

- installed MSI auto-update
- macOS `.app` replacement
- AppImage replacement
- silent/background update
- production auto-update rollout

See [docs/updater/portable-update-implementation.md](../updater/portable-update-implementation.md).

## Builds may be unsigned

Use SHA256 checksums to verify downloads.

## No production installer yet

Portable/bundle artifacts are the **primary 1.0.0 stable distribution format**.

Windows has an **MSI PoC** for testing installed layout — see [docs/installer/windows-msi-poc.md](../installer/windows-msi-poc.md). It is not a production-signed installer and must not install the helper service by default.
