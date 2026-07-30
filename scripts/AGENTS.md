# AGENTS.md — `scripts/`

Build, package, sign, smoke, and release helpers. Prefer these over ad-hoc cmake/env on Windows.

## Common

| Script | Use |
|--------|-----|
| `configure-msvc2026.ps1` | Configure local/release (static Qt + required lib_ui) |
| `build.ps1` | Configure+build wrapper (static Qt; `-Test`, `-Force`) |
| `build-macos.sh` | macOS configure+build wrapper (Homebrew `qt@6`; `--test`, `--force`) |
| `build-qt-static-msvc2026.ps1` | Static Qt (qtbase+qtsvg) → `C:\Qt\Static\6.8.3\msvc2022_64`; `-SvgOnly` adds Svg to an existing prefix |
| `pack-qt-static-ci.ps1` | Pack static Qt prefix to `dist/qt-static-*-msvc2022_64.7z` for CI |
| `setup-msvc-actions.ps1` | Enable the hosted-runner MSVC environment without a Node action |
| `run-xray-config-test.ps1` | Xray config unit tests |
| `run-kde-proxy-smoke.sh` | Isolated native KDE/KConfig proxy apply/restore smoke |
| `check-translations.py` | Translation completeness (CI) |
| `package-windows.ps1` | Portable ZIP packaging (bundles pinned Xray by default) |
| `package-windows-msi.ps1` | WiX MSI PoC |
| `package-linux.sh` / `package-macos.sh` | Non-Windows packages |
| `run-smoke-tests.py` / `smoke-*.ps1|sh` | Artifact smoke checks |
| `sign-*.ps1|sh` | Platform signing hooks |
| `audit-redaction.py` | Redaction audit for release |

## Rules

- Windows local/release/CI app builds: **static Qt only** (`QT_STATIC_DIR` / `C:\Qt\Static\…`).
- Do not hardcode machine-specific Qt paths without `QT_STATIC_DIR` override.
- Packaging must not ship secrets, forbidden files, or unredacted diagnostics samples.
- Keep scripts non-interactive and CI-friendly (flags over prompts).

Docs: `docs/release-packaging.md`, `docs/signing/`, `docs/updater/`.
