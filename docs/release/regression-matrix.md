# Zarya 1.5.0 Stable Regression Matrix

Automated rows are release gates. Desktop interaction rows use the evidence
procedure in `docs/platform-test-matrix.md`.

## Cross-platform automated

- [x] all six stable protocols accept representative valid links
- [x] invalid/malformed links fail without exposing credentials
- [x] IPv6, duplicate query key, unsupported option, and mixed subscription
  cases pass
- [x] failed subscription refresh preserves existing profiles
- [x] imported profiles persist and reload compatibly
- [x] stable feature gating and version tests pass
- [x] release archive audit rejects user data and likely runtime secrets

## Windows 11 x64 — primary

- [ ] fresh portable start and first-run wizard
- [ ] bundled/detected Xray starts a valid profile
- [x] system-proxy apply/restore behavior covered by `system_proxy`
- [ ] subscription update and node tests
- [ ] backup export/import and diagnostics bundle
- [ ] tray close/restore/exit

## Windows 10 x64

Repeat the Windows 11 essential path when hardware is available.

## macOS arm64

- [ ] app and bundled/detected Xray start
- [ ] `networksetup` proxy apply/restore
- [ ] subscription update, backup, and diagnostics

## Linux Ubuntu GNOME

- [ ] app and bundled/detected Xray start
- [x] complete gsettings snapshot/apply/restore covered by `linux_proxy`
- [ ] desktop subscription update, backup, and diagnostics

## Linux KDE

- [x] KConfig 6 and KConfig 5 backend selection covered
- [x] HTTP/HTTPS apply and KIO reload covered
- [x] exact stop/recovery restore and rollback covered
- [x] missing native facilities never claim proxy-on
- [x] isolated native KConfig 5/6 and session-D-Bus evidence recorded

## Experimental matrix (non-blocking)

Run separately when experimental features are explicitly enabled:

- [ ] TUN start/stop
- [ ] helper service controls
- [ ] kill switch enable/disable/recovery
- [ ] MSI PoC install
- [ ] app updater install PoC
