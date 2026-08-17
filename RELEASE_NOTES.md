# Zarya 1.5.13

Zarya 1.5.13 prepares the Windows LCL cutover: Windows CI/release jobs build
the single-file LCL artifact, strict two-file packaging verification is
enforced, and SignPath signs only `Zarya.exe` with fail-closed checks.

See [docs/release-notes/1.5.13.md](docs/release-notes/1.5.13.md) for details.

---

# Zarya 1.5.12

Zarya 1.5.12 adds reproducible LCL Windows release engineering, a pinned
external-core integration matrix, and a verified two-file portable package.

See [docs/release-notes/1.5.12.md](docs/release-notes/1.5.12.md) for details.

---

# Zarya 1.5.11

Zarya 1.5.11 extracts the LCL runtime, profile, and background-operation
services from `MainForm` without changing stable behavior.

See [docs/release-notes/1.5.11.md](docs/release-notes/1.5.11.md) for details.

---

# Zarya 1.5.10

Zarya 1.5.10 completes the LCL first-run and built-in EN/RU stable UI while
keeping experimental runtime features hidden by default.

See [docs/release-notes/1.5.10.md](docs/release-notes/1.5.10.md) for details.

---

# Zarya 1.5.9

Zarya 1.5.9 adds the LCL Windows settings and lifecycle foundation, including
safe autostart of an existing compatible profile.

See [docs/release-notes/1.5.9.md](docs/release-notes/1.5.9.md) for details.

---

# Zarya 1.5.8

Zarya 1.5.8 hardens the LCL Qt-data migration, rollback, backup, and
diagnostics privacy contracts.

See [docs/release-notes/1.5.8.md](docs/release-notes/1.5.8.md) for details.

---

# Zarya 1.5.7

Zarya 1.5.7 adds the versioned LCL routing and DNS model, capability-checked
provider mappings, native policy editors, and atomic user geodata management.

See [docs/release-notes/1.5.7.md](docs/release-notes/1.5.7.md) for details.

---

# Zarya 1.5.6

Zarya 1.5.6 introduces the isolated LCL runtime worker foundation, Xray ABI v2,
dynamic test ports, and bounded Real delay execution without changing the
stable Qt client path.

See [docs/release-notes/1.5.6.md](docs/release-notes/1.5.6.md) for details.

---

# Zarya 1.5.5

Zarya 1.5.5 fixes isolated embedded Xray delay tests by keeping worker stdout JSON-only and tolerating non-protocol diagnostic prefixes.

See [docs/release-notes/1.5.5.md](docs/release-notes/1.5.5.md) for details.

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
