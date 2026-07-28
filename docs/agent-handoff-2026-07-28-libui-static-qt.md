# Agent handoff — lib_ui mandatory + Windows static Qt (2026-07-28)

Use this doc to continue work in a **new agent session**. Trunk is clean; no open feature branch for this work.

## Snapshot

| Item | Value |
|------|--------|
| Repo | `vladon/zarya` |
| Branch | `main` @ `482ae24` (sync: `git pull origin main`) |
| App version | **1.3.0** (`stable` channel) |
| Closed PRs | [#86](https://github.com/vladon/zarya/pull/86) lib_ui mandatory + static Windows CI; [#87](https://github.com/vladon/zarya/pull/87) quiet ZLIB/JPEG find |
| Prior related | [#85](https://github.com/vladon/zarya/pull/85) status strip on lib_ui / Desktop App UI default ON for Windows static |
| CI Qt asset | Release [`ci-qt-static-6.8.3`](https://github.com/vladon/zarya/releases/tag/ci-qt-static-6.8.3) → `qt-static-6.8.3-msvc2022_64.7z` (~31 MB packed) |
| Local static Qt | `C:\Qt\Static\6.8.3\msvc2022_64` (`QT_STATIC_DIR` override) |
| Local build | `.\scripts\build.ps1` — verified OK after #86/#87 |

**Working tree note:** ignore untracked `.claude/` / `.codex/` — do not commit.

**Transcript (optional):** Cursor agent transcript `14fff186-ba4d-42d5-8b97-42aed39ccfb9` (lib_ui / static Qt saga).

---

## What was decided (do not reopen lightly)

1. **`lib_ui` (Desktop App Toolkit) is mandatory** on all platforms. No MIT-only / no-lib_ui configure path. Official binaries are **GPLv3+** because of the toolkit; Zarya-authored source stays dual-licensed (see `LICENSE`, `README`, `THIRD_PARTY_NOTICES.md`).
2. **Windows = static Qt only.** Shared/aqt Qt for Windows app builds is unsupported. CMake `FATAL_ERROR` if Qt is not static / `ZARYA_STATIC_QT` off on `WIN32`.
3. **Linux / macOS CI** keep dynamic Qt (aqt / brew). Only Windows CI consumes the packed static prefix.
4. Shared-Qt CI workarounds were **removed** after switching to static (DEPENDENTLOADFLAG clear, PATH `.cmd` codegen launchers, `cmake/desktop_app_generate/` wrappers). Do not bring them back for Windows.

---

## Architecture reminder

```
UI → AppController → IRuntimeBackend (Xray system-proxy | sing-box TUN)
                   → CoreManager → external xray/sing-box
Helper: GUI → helperclient/ipc → zarya-helper (elevated TUN / kill switch)
```

- Stable: Xray system-proxy. Experimental (gated): TUN, helper, kill switch, updater install — `FeatureGate` / `docs/stable/`.
- `namespace zarya` for app code. Nested `AGENTS.md` under key `src/` trees.

---

## Build (Windows)

```powershell
git submodule update --init --recursive
.\scripts\configure-msvc2026.ps1   # always static; no -Static / -Shared
cmake --build build --config Release --target zarya
# or:
.\scripts\build.ps1
```

- Prefix: `C:\Qt\Static\6.8.3\msvc2022_64` (`QT_STATIC_DIR`).
- Build Qt once: `.\scripts\build-qt-static-msvc2026.ps1` (qtbase+qtsvg, `/MT` via `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`).
- OpenSSL: prefer `C:\Program Files\OpenSSL-Win64` with **MT** static libs (`OPENSSL_USE_STATIC_LIBS` / `OPENSSL_MSVC_STATIC_RT` when static Qt). Fallback in configure: `third_party/desktop-app/openssl-win64`.
- Missing Svg in kit → stubs under `cmake/desktop_app_stubs/`.
- Cursor rule: `.cursor/rules/static-build.mdc`. Script notes: `scripts/AGENTS.md`.

**macOS:** `./scripts/build-macos.sh` (Homebrew `qt@6`).

**Tests:**

```powershell
.\scripts\run-xray-config-test.ps1
.\build\Release\zarya_subscription_test.exe
ctest --test-dir build -C Release
```

---

## CI — Windows static Qt

### Consumers

- [`.github/workflows/build.yml`](../.github/workflows/build.yml) — `build-windows`
- [`.github/workflows/release.yml`](../.github/workflows/release.yml) — `release-windows`

Flow:

1. `actions/cache` key: `qt-static-6.8.3-msvc2022_64-v1` → path `C:\Qt\Static\6.8.3\msvc2022_64`
2. On miss: `gh release download ci-qt-static-6.8.3` → extract `qt-static-6.8.3-msvc2022_64.7z` into `C:\Qt\Static\6.8.3\` (archive root leaf is `msvc2022_64/`)
3. Save cache after download
4. Configure: `-DZARYA_STATIC_QT=ON -DCMAKE_PREFIX_PATH=...` + OpenSSL (choco), Ninja, MSVC via `ilammy/msvc-dev-cmd`
5. **No** aqt shared Qt, **no** vcpkg jpeg/zlib for Windows app CI anymore

### Bootstrap / refresh static Qt asset

- Pack local prefix: `.\scripts\pack-qt-static-ci.ps1` → `dist/qt-static-6.8.3-msvc2022_64.7z` (`dist/` is gitignored)
- Or workflow_dispatch: [`.github/workflows/qt-static-windows.yml`](../.github/workflows/qt-static-windows.yml) (long cold build)
- Publish/update release tag `ci-qt-static-6.8.3` (not a product `v*` tag)
- `build-qt-static-msvc2026.ps1` accepts `QT_VCVARS` for GHA Enterprise VS paths

### YAML pitfalls already hit

- Do **not** put PowerShell here-strings `@"..."@` inside workflow `run:` blocks — breaks Actions YAML parse (0s failed runs).
- Quote Windows paths in workflow `env:`: `QT_STATIC_DIR: 'C:\Qt\Static\...'`.
- Do **not** use bash `export PATH=D:\...` — backslashes corrupt PATH.

---

## lib_ui / desktop-app integration (keep)

| Area | Notes |
|------|--------|
| Force ON | [`cmake/ZaryaDesktopAppUi.cmake`](../cmake/ZaryaDesktopAppUi.cmake) — always ON (`FORCE`); no OFF early-return |
| Externals | [`cmake/ZaryaDesktopAppExternals.cmake`](../cmake/ZaryaDesktopAppExternals.cmake) — zlib/jpeg → system or `Qt6::BundledZLIB` / `Qt6::BundledLibjpeg` |
| Quiet find | `find_package(ZLIB/JPEG QUIET)` — avoid configure spam when only Qt bundled exists (#87) |
| Linux | Vendored portal XMLs; submodule `third_party/desktop-app/kcoreaddons` @ KDE `v6.11.0`; glib + `Qt6::DBus` |
| macOS | `enable_language(OBJC/OBJCXX)`; stub `AGL.framework`; strip AGL from OpenGL INTERFACE |
| Qt version gates | Build-tree patches: `QAccessible::Attribute::Orientation` at **Qt 6.10+**; `Qt::NoTitleBarBackgroundHint` at **Qt 6.9+** (`ui_window_mac.mm`) |
| MSVC PCH | `/FI` on MSVC vs `-include` on GCC/Clang for desktop-app sources |
| UI forks | `#if ZARYA_DESKTOP_APP_UI` removed from Application / MainWindow / StatusBadge / StatusDashboardWidget |

Submodules: `git submodule update --init --recursive` (includes `third_party/desktop-app/*`).

---

## Hard-won failures (do not regress)

1. **`DEPENDENTLOADFLAG:0x800`** (`LOAD_LIBRARY_SEARCH_SYSTEM32`) from desktop-app `options_win.cmake` — with **shared** Qt, `codegen_style` died `0xC0000135` (DLL not found) even with PATH/windeploy. Fixed historically by clearing the flag for shared builds; **obsolete on Windows** now that CI is static-only (tools don’t need Qt DLLs the same way).
2. **`libcrypto_static` vs shared Qt `/MD`** — CRT mismatch; reason Windows moved to **static Qt `/MT`** instead of more shared patches.
3. Bash PATH with `D:\` escapes — use **pwsh** for PATH / build steps on Windows CI.
4. Invalid workflow YAML → Actions run fails in **0s** with empty jobs; check annotations on the run page.

---

## Agent / git workflow

- Trunk: `main`. Never push `main` directly.
- Branch `feat/` / `fix/` / `chore/` → PR → `gh pr merge` → sync local `main`, delete branch.
- Do **not** commit unless the user asks. Prefer minimal diffs.
- Landing default (when user asks to finish/ship): commit on branch + PR + merge (see `.cursor/rules/trunk-based-git.mdc`).
- Secrets: never commit/log proxy URLs, tokens, credentials.
- i18n: `tr()` / `ZaryaTr::tr()`; update `translations/zarya_en.ts` + `zarya_ru.ts`.

---

## Suggested next work (user had not chosen yet)

Pick one when starting the new session:

1. **Stable release hygiene** — `.\scripts\package-windows.ps1`, verify portable ZIP, smoke; optional tag `v*` / release notes under `docs/release-notes/`, `docs/release/`.
2. **Experimental track** — sing-box TUN, `zarya-helper`, kill switch, updater (`docs/tun-design.md`, `docs/kill-switch-design.md`, `docs/updater/`, `docs/stable/feature-gating.md`). Respect `FeatureGate`.
3. **CI polish** — Node 20 deprecation warnings on Actions; bump `actions/checkout` etc. when convenient.
4. **Product / UI** — concrete bug or feature from backlog (`docs/stable/1.0-backlog.md` and related).

---

## Key file index

| Path | Role |
|------|------|
| `AGENTS.md` | Project agent entry |
| `.cursor/rules/static-build.mdc` | Windows static-only rule |
| `.cursor/rules/trunk-based-git.mdc` | Branch/PR/merge |
| `cmake/ZaryaDesktopAppUi.cmake` | lib_ui always ON, OpenSSL/ZLIB/JPEG, Qt modules |
| `cmake/ZaryaDesktopAppExternals.cmake` | zlib/jpeg/lz4/openssl wiring for toolkit |
| `scripts/configure-msvc2026.ps1` / `build.ps1` | Local Windows configure/build |
| `scripts/build-qt-static-msvc2026.ps1` | Build static Qt prefix |
| `scripts/pack-qt-static-ci.ps1` | Pack prefix for CI release |
| `.github/workflows/build.yml` | PR/main CI (Windows static) |
| `.github/workflows/release.yml` | Tag release Windows static |
| `.github/workflows/qt-static-windows.yml` | Bootstrap/refresh static Qt asset |
| `docs/stable/` | Stable scope / gating |
| `docs/release-packaging.md` | Packaging |

---

## Prompt starter for the next session

```text
Read docs/agent-handoff-2026-07-28-libui-static-qt.md and AGENTS.md.
Repo is on main after #86/#87 (lib_ui mandatory, Windows static Qt only, CI asset ci-qt-static-6.8.3).
Continue with: <RELEASE | TUN/KS | CI polish | FEATURE X>.
Follow trunk-based PRs; do not commit until I ask.
```
