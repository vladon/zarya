# Zarya 1.5.0

Zarya 1.5.0 brings full KDE Plasma system-proxy support and hardens the stable
desktop path against partial proxy changes, failed subscription refreshes, and
diagnostic leaks.

## Highlights

- KDE Plasma 5/6 proxy snapshot, apply, KIO reload, exact restore, and rollback
- Consistent proxy status/recovery UX across Windows, macOS, GNOME, and KDE
- Regression coverage for all six stable share-link protocols
- Non-destructive subscription refresh failures
- Full CTest, packaging verification, and archive secret audit on every release
  platform
- Pinned Xray and runetfreedom geo data retained for offline first start

## Recommended mode

Use Xray system-proxy mode.

## Downloads

- Windows x64 portable ZIP
- macOS arm64 ZIP
- Linux x64 tar.gz

Verify the archive using its `.sha256` sidecar or `SHA256SUMS.txt`.

## Licensing

Zarya-authored source is MIT or GPLv3+. Official binaries link Desktop App
Toolkit and are GPLv3+.

## Experimental features

TUN, helper, kill switch, MSI, and app-update installation remain experimental
and are disabled or hidden by default.

## Known limitations

System-proxy mode is not VPN mode. Some applications may ignore operating-system
proxy settings. KDE support requires KConfig tools and session D-Bus.

## Issue reports

Use **Help → Create Diagnostics Bundle**. Do not post raw proxy links, private
keys, credentials, or subscription URLs.
