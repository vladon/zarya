# Zarya 1.5.1

Zarya 1.5.1 embeds both proxy cores: Xray runs in-process in the GUI and sing-box runs in-process only inside the privileged `zarya-helper` for experimental TUN. No external core executable is included in the release artifact.

See [docs/release-notes/1.5.1.md](docs/release-notes/1.5.1.md) for details.

---

# Zarya 1.5.0
## Stable release

1.5.0 adds full KDE Plasma system-proxy integration and strengthens proxy
recovery, profile import, diagnostics redaction, and release verification.

- Apply and exactly restore KDE Plasma 5/6 proxy and PAC settings through
  native KConfig tools, with rollback if a write or KIO reload fails
- Use consistent system-proxy status and recovery behavior on Windows, macOS,
  GNOME, KDE, and unsupported Linux desktops
- Protect all six stable share-link protocols with valid, invalid,
  persistence, mixed-subscription, and secret-redaction regressions
- Preserve existing profiles when a subscription refresh fails
- Run full CTest, package verification, and archive secret audits on Windows,
  Linux, and macOS
- Continue shipping pinned Xray and runetfreedom geo data for offline first
  start

See [docs/release-notes/1.5.0.md](docs/release-notes/1.5.0.md) and
[docs/stable/stable-scope.md](docs/stable/stable-scope.md).

## Recommended path

Xray system-proxy mode (stable scope).

## Reporting issues

Use **Help → Create Diagnostics Bundle** or **Help → Copy Support Summary**.