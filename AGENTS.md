# AGENTS.md — Zarya

Zarya is a cross-platform desktop client for proxy profiles and external cores (Xray, sing-box). Two stacks share this repo:

- **Windows app = LCL / Free Pascal** in `prototypes/lcl/` — despite the directory name this is the shipping Windows app. Since 1.5.13 the Windows release is a single-EXE portable ZIP containing exactly `Zarya.exe` + `Zarya.exe.sha256`.
- **Qt 6 / C++20 app** in `src/` (all C++ in `namespace zarya`) — Linux/macOS builds and the behavior reference for LCL. The Windows Qt static build is legacy; its removal is planned (`chore/remove-windows-qt`, see handoff doc below).

**Version:** `cmake/ZaryaVersion.cmake` is the only source of truth — read it, never invent or hardcode version numbers. Channel: `stable`.
**License:** dual MIT | GPLv3+ for Zarya-authored source (`LICENSE`, `LICENSE.MIT`, `LICENSE.GPL-3.0`).
**Stable path:** embedded Xray + system proxy. **Experimental (gated):** sing-box TUN, helper, kill switch, self-update. Release signing is optional; artifacts ship unsigned with mandatory SHA-256 (SignPath activates only when `SIGNPATH_ENABLED=true`).

## Workflow

- Trunk is `main`; never push to it. Branch `feat/` / `fix/` / `chore/`, open a PR: one logical change, include a test plan.
- Every merged PR bumps the patch version in `cmake/ZaryaVersion.cmake`.
- Do not commit unless the user asks. Prefer minimal diffs; match surrounding style (no repo-wide clang-format).
- Before Windows release work, read `docs/agent-handoff-2026-08-17-lcl-cutover.md` — it lists stable behavior that must not change (readiness polling before proxy enable, no silent provider fallback, worker protocol, redaction) and pending follow-ups.

## Windows (LCL — primary release path)

Pinned toolchain: Lazarus 4.8, FPC 3.2.2, Go 1.26.5, WinLibs MinGW-w64 GCC 16.1.0.

```powershell
cd prototypes\lcl
.\bootstrap-toolchain.ps1   # one-time / clean machine; SHA-256-verified download
.\build.ps1                 # Go c-archive + Pascal units -> bin\Zarya.exe (+ .sha256)
.\test.ps1                  # stores, parsers, backup/redaction, embedded Xray runtime tests
.\test-known-cores.ps1      # pinned external-core fixtures into generated\ only (never shipped)
.\package.ps1               # exact two-file release ZIP into dist\
```

- `build.ps1` statically links the shared Go bridge from `src\runtime\embedded\xray\bridge`; a plain Lazarus F9 build does **not** produce the production EXE.
- MinGW for cgo defaults to `build\tools\winlibs`; override with `CC`.
- Development geodata comes from `build\Release\cores\xray`; packaged layout is `bin\cores\xray`.
- LCL first run imports Qt data `%LOCALAPPDATA%\Zarya\Zarya` → `%LOCALAPPDATA%\Zarya\LCL` (checksum ZIP → staging → atomic install; originals untouched; skipped for `--portable` / `--data-dir`).

## Qt app (Linux/macOS; legacy on Windows)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target zarya zarya_tests   # zarya_tests = meta target for all test exes
ctest --test-dir build --output-on-failure
./scripts/build-macos.sh                          # macOS wrapper: --test, --force, --config, --target
```

- ctest runs: xray_config, singbox_config, subscription, embedded_xray_runtime, geodata_verifier, core_checksum, ui_animation_policy, ui_theme_contrast, ui_accessibility, smoke, version, stable_hardening (+ system_proxy on Windows Qt, linux_proxy on Linux). Single test: run the `zarya_<name>_test` executable directly, e.g. `.\\build\\Release\\zarya_subscription_test.exe`.
- lib_ui (Desktop App Toolkit) is vendored under `third_party/desktop-app/` and always linked; new Qt UI must use lib_ui — `scripts/check-libui-boundary.py` enforces a shrinking allowlist of legacy Qt visual controls in CI.
- Legacy Windows Qt builds require **static Qt only** (`C:\Qt\Static\6.8.3\msvc2022_64`, `QT_STATIC_DIR` overrides; shared Qt fails configure on WIN32): `.\scripts\configure-msvc2026.ps1`, `.\scripts\build.ps1`, one-time `.\scripts\build-qt-static-msvc2026.ps1` (`-SvgOnly` adds Svg to an existing prefix; Svg-less kits use stubs in `cmake/desktop_app_stubs/`).
- Targets: `zarya` (GUI), `zarya-helper`, `zarya-updater`, `zarya-core-test-worker`.

## Go bridges (shared by both stacks)

`src/runtime/embedded/xray/bridge` and `src/runtime/embedded/singbox/bridge` are vendored Go modules (embedded Xray via cgo). Verify any change with `go mod verify && go test ./...` (Go 1.26.5); on Windows point `CC` at the MinGW GCC. CI runs these on every push.

## CI (every PR / push to `main`)

- `scripts/check-libui-boundary.py` + shrinking-allowlist diff vs base sha
- `tests/release_signing_contract_test.py` (plain Python, run directly)
- `scripts/check-translations.py` (EN/RU catalog completeness)
- Go bridge `go mod verify` + `go test ./...`
- Windows job: LCL build/test/package + `scripts/verify-release-artifacts.py --windows-lcl-single-exe` + `scripts/audit-release-artifact.py`
- Ubuntu/macOS jobs: Qt build + ctest (+ KDE proxy smoke on Ubuntu) + package verify/audit

## Agent rules

1. **Feature gating** — experimental surfaces go through `features/FeatureGate`; stable channel hides them and the effective runtime falls back to Xray system-proxy.
2. **i18n** — Qt: `tr()` in `QObject`s, `ZaryaTr::tr()` elsewhere; update `translations/zarya_en.ts` and `zarya_ru.ts`. LCL: built-in EN/RU tables in `ZaryaTr.pas`. Never translate protocol/core names or error codes. `check-translations.py` runs in CI.
3. **Secrets** — never commit or log proxy/subscription URLs, tokens, or credentials. Diagnostics/backups redact by default; `audit-release-artifact.py` gates releases.
4. **Privilege** — keep the GUI unprivileged; elevated work goes through `zarya-helper` (Qt app).
5. **External cores are user-owned** — Zarya and official artifacts never download, back up, or distribute them; CI downloads pinned fixtures only (`prototypes/lcl/known-cores.json` → `generated\`).
6. **Portable** — `--portable` or `portable.flag` → data under `./data`.

## Where things live

- `src/core/` — Xray config/adapter (**not** binary updates); `src/cores/` — core binary download/update manager
- `src/runtime/` — `IRuntimeBackend` + Xray / sing-box backends; `src/runtime/embedded/` — Go bridges
- `src/helper/`, `src/helperclient/`, `src/ipc/`, `src/service/` — privileged helper + IPC
- Nested `AGENTS.md`: `src/` (plus `core/`, `cores/`, `runtime/`, `helper/`, `features/`, `platform/`), `scripts/`, `docs/`

## Key docs

| Topic | Path |
|-------|------|
| Stable scope / gating | `docs/stable/` |
| Windows LCL cutover, handoff | `docs/agent-handoff-2026-08-17-lcl-cutover.md`, `prototypes/lcl/MIGRATION.md` |
| Security / redaction | `docs/security-model.md` |
| Helper | `docs/privileged-helper-design.md`, `docs/service/` |
| TUN / kill switch | `docs/tun-design.md`, `docs/kill-switch-design.md` |
| Localization | `docs/localization.md` |
| Packaging / updater / signing | `docs/release-packaging.md`, `docs/updater/`, `docs/signing/` |
| Contributing | `CONTRIBUTING.md` |

Cursor rules: `.cursor/rules/trunk-based-git.mdc`, `.cursor/rules/static-build.mdc`.
