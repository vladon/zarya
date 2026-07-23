# AGENTS.md — `scripts/`

Build, package, sign, smoke, and release helpers. Prefer these over ad-hoc cmake/env on Windows.

## Common

| Script | Use |
|--------|-----|
| `configure-msvc2026.ps1 -Static` | Configure local/release (static Qt) |
| `build.ps1` | Configure+build wrapper (static default; `-Shared`, `-Test`) |
| `build-qt-static-msvc2026.ps1` | One-time static Qt → `C:\Qt\Static\6.8.3\msvc2022_64` |
| `run-xray-config-test.ps1` | Xray config unit tests |
| `check-translations.py` | Translation completeness (CI) |
| `package-windows.ps1` | Portable ZIP packaging |
| `package-windows-msi.ps1` | WiX MSI PoC |
| `package-linux.sh` / `package-macos.sh` | Non-Windows packages |
| `run-smoke-tests.py` / `smoke-*.ps1|sh` | Artifact smoke checks |
| `sign-*.ps1|sh` | Platform signing hooks |
| `audit-redaction.py` | Redaction audit for release |

## Rules

- Windows local/release: static Qt via `-Static` unless user requests `-Shared`.
- Do not hardcode machine-specific Qt paths without `QT_STATIC_DIR` / `QT_ROOT` overrides.
- Packaging must not ship secrets, forbidden files, or unredacted diagnostics samples.
- Keep scripts non-interactive and CI-friendly (flags over prompts).

Docs: `docs/release-packaging.md`, `docs/signing/`, `docs/updater/`.
