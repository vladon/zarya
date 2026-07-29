# Zarya 1.5.0 Stable Go / No-Go

## Required

- [x] version set to 1.5.0
- [x] channel remains stable
- [x] release PR checks pass on Windows, macOS, and Linux
- [x] portable Windows artifact verified and secret-audited
- [x] macOS artifact verified and secret-audited
- [x] Linux artifact verified and secret-audited
- [x] release workflow checksums and manifests verified after tagging
- [x] stable feature gating covered by `stable_hardening`
- [x] system-proxy restore and rollback covered on Windows/GNOME/KDE
- [x] import/subscription failure and redaction regressions pass
- [x] KDE KConfig 5/6 and isolated KIO signal evidence recorded
- [x] no open critical or high blocker in the stable path (GitHub issues,
  checked 2026-07-29)
- [x] release notes and GitHub release draft complete

## Decision

- [x] Go
- [ ] No-go

## Evidence

- Release readiness: `docs/release/1.5.0-checklist.md`
- Regression matrix: `docs/release/regression-matrix.md`
- Release PR #97: final GitHub Actions run `30495897356` (Windows 20m21s,
  Ubuntu 13m44s, macOS 11m47s).
- Tag `v1.5.0`: `ce3dfd7e276449cf6eca7a003e3c9ecd911bd48e`.
- Release workflow: GitHub Actions run `30497172838`; all three platform jobs
  passed their tests, package verification, and secret audit.
- Published release:
  <https://github.com/vladon/zarya/releases/tag/v1.5.0> (Latest, seven assets).
- Post-publication readback matched both the combined `SHA256SUMS.txt` and all
  three per-archive checksum sidecars.
- Interactive desktop rows that remain unchecked in the regression matrix were
  not represented as executed. The Go decision accepts that residual risk
  based on the passing cross-platform CI and native KDE KConfig/KIO evidence.
