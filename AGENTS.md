# AGENTS.md — Zarya

Zarya is a cross-platform **Qt 6 / C++20** desktop client for proxy profiles and external cores (**Xray**, **sing-box**). Current version: **1.0.0** (`stable` channel).

**Stable path:** Xray system-proxy (profiles, subscriptions, routing/DNS, geo data, tray, EN/RU UI).  
**Experimental (gated):** sing-box TUN, `zarya-helper`, kill switch, app update install. See `docs/stable/`.

## Workflow

- Trunk is `main`. Never push to `main`. Branch `feat/` / `fix/` / `chore/`, open a PR.
- One logical change per PR. Include a test plan.
- Do not commit unless the user asks.
- Prefer minimal diffs; match existing style. All app code is in `namespace zarya`.

## Build (Windows primary)

Local/release builds use **static Qt** unless the user asks for shared:

```powershell
.\scripts\configure-msvc2026.ps1 -Static
cmake --build build --config Release --target zarya
# or: .\scripts\build.ps1          # static by default
#      .\scripts\build.ps1 -Shared  # faster iteration
```

- Static Qt prefix: `C:\Qt\Static\6.8.3\msvc2022_64` (`QT_STATIC_DIR` to override).
- One-time static Qt build: `.\scripts\build-qt-static-msvc2026.ps1`
- Do not change CMake defaults for static linking; use `-Static` / `ZARYA_STATIC_QT=ON` via the configure script.
- CI uses shared Qt. Use `build/` for local work (ignore `build-ci-test*`).

**Targets:** `zarya` (GUI), `zarya-helper`, `zarya-updater` (copied next to `zarya` post-build).

**Tests:**

```powershell
.\scripts\run-xray-config-test.ps1
.\build\Release\zarya_subscription_test.exe
ctest --test-dir build -C Release   # smoke, version, stable_hardening
```

Xray/sing-box binaries are **not** in the repo; place under `cores/xray/` or `cores/sing-box/` (or Settings). The app runs without cores for offline profile work.

## Architecture (short)

```
UI → AppController → IRuntimeBackend (Xray system-proxy | sing-box TUN)
                   → CoreManager → external xray/sing-box
Helper: GUI → helperclient/ipc → zarya-helper (elevated TUN / kill switch)
```

- `src/core/` — Xray config/adapter (`ICoreAdapter`, `XrayConfigBuilder`)
- `src/cores/` — **binary** download/update manager (not the same as `core/`)
- `src/features/` — `FeatureGate` / release-channel gating
- Nested `AGENTS.md` under key `src/` and `scripts/` / `docs/` folders for module detail

## Agent rules

1. **Feature gating** — Respect `FeatureGate`. On stable, experimental UI is hidden; effective runtime falls back to Xray system-proxy. Recovery for kill switch / system proxy stays available.
2. **i18n** — User-facing strings: `tr()` in `QObject`s, `ZaryaTr::tr()` elsewhere. Update `translations/zarya_en.ts` and `zarya_ru.ts`. Do not translate protocol/core names or error codes. See `docs/localization.md`.
3. **Secrets** — Never commit or log proxy/subscription URLs, tokens, or credentials. Diagnostics/backups redact by default.
4. **Privilege** — Keep the GUI unprivileged; elevated work goes through `zarya-helper`.
5. **Portable** — `--portable` or `portable.flag` → data under `./data`, cores under `./cores/`.

## Key docs

| Topic | Path |
|-------|------|
| Stable scope / gating | `docs/stable/` |
| Security | `docs/security-model.md` |
| Helper | `docs/privileged-helper-design.md`, `docs/service/` |
| TUN / kill switch | `docs/tun-design.md`, `docs/kill-switch-design.md` |
| Localization | `docs/localization.md` |
| Packaging / updater | `docs/release-packaging.md`, `docs/updater/` |
| Contributing | `CONTRIBUTING.md` |

Cursor rules: `.cursor/rules/trunk-based-git.mdc`, `.cursor/rules/static-build.mdc`.
