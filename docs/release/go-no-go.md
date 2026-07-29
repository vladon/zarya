# Zarya 1.5.0 Stable Go / No-Go

## Required

- [x] version set to 1.5.0
- [x] channel remains stable
- [ ] release PR checks pass on Windows, macOS, and Linux
- [ ] portable Windows artifact verified and secret-audited
- [ ] macOS artifact verified and secret-audited
- [ ] Linux artifact verified and secret-audited
- [ ] release workflow checksums and manifests verified after tagging
- [x] stable feature gating covered by `stable_hardening`
- [x] system-proxy restore and rollback covered on Windows/GNOME/KDE
- [x] import/subscription failure and redaction regressions pass
- [x] KDE KConfig 5/6 and isolated KIO signal evidence recorded
- [x] no open critical or high blocker in the stable path (GitHub issues,
  checked 2026-07-29)
- [x] release notes and GitHub release draft complete

## Decision

- [ ] Go
- [ ] No-go

## Evidence

- Release readiness: `docs/release/1.5.0-checklist.md`
- Regression matrix: `docs/release/regression-matrix.md`
- CI and release URLs are recorded after the corresponding workflows complete.
