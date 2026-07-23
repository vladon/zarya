# AGENTS.md — `src/`

Application source. Namespace: `zarya`. Prefer editing the module that owns the concern; keep platform code under `platform/<os>/`.

## Modules

| Dir | Role |
|-----|------|
| `app/` | `main`, `Application`, `AppController`, startup |
| `ui/` | Widgets, dialogs, tray, models, onboarding |
| `domain/` | `Profile`, routing/DNS/subscription types |
| `core/` | Xray adapter + config builder (**not** binary updates) |
| `cores/` | Download/install/verify Xray & sing-box binaries |
| `runtime/` | `IRuntimeBackend` + Xray / sing-box backends |
| `subscription/`, `import/` | Subscriptions, VLESS URI parse |
| `routing/`, `dns/` | Routing/DNS managers + Xray generators |
| `geodata/`, `rulesets/` | Geo `.dat` and sing-box `.srs` |
| `platform/` | System proxy, autostart (per-OS + stubs) |
| `storage/` | JSON persistence, `AppPaths` |
| `helper/`, `helperclient/`, `ipc/` | Privileged helper + client + transport |
| `service/` | OS service install for helper |
| `killswitch/` | Experimental kill switch (nft / WFP / stubs) |
| `updater/` | App self-update + `zarya-updater` |
| `features/` | Channel-based feature gating |
| `backup/`, `diagnostics/` | Backup ZIP, support bundle (redact secrets) |
| `security/`, `recovery/`, `migration/` | Redaction, crash recovery, schema migrations |
| `i18n/`, `errors/`, `logging/` | Translations helpers, errors, log buffer |
| `testing/` | Latency / port helpers used by UI |
| `packaging/` | Install mode / portable migration helpers |

## Conventions

- C++20, Qt Core/Gui/Widgets/Network. Match surrounding file style (no repo-wide clang-format).
- Domain types stay free of UI; persistence goes through `storage/`.
- New user-facing strings need EN+RU translation updates.
- Experimental surfaces must go through `features/FeatureGate`.

See nested `AGENTS.md` in `core/`, `cores/`, `runtime/`, `helper/`, `features/`.
