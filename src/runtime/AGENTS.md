# AGENTS.md — `src/runtime/`

Runtime backends: how a started profile is actually run.

## Model

- `IRuntimeBackend` + `RuntimeBackendFactory`
- `runtime/xray/` — **stable default**: Xray + OS system proxy
- `runtime/singbox/` — **experimental**: sing-box TUN (+ rule-set context)

`configuredRuntimeMode` (settings) vs `effectiveRuntimeMode` (after `FeatureGate`). On stable with experimental hidden, effective mode is Xray system-proxy; stored TUN settings are kept, not deleted.

## Rules

- Prefer Xray system-proxy for stable behavior changes.
- TUN paths depend on `zarya-helper` / privilege; do not assume elevation in the GUI process.
- Config generators should stay testable without a live core (`zarya_xray_config_test`, `zarya_singbox_config_test`).
- Surface clear config warnings via existing warning types rather than new ad-hoc UI.

Docs: `docs/tun-design.md`, `docs/stable/feature-gating.md`, `docs/public-beta/experimental-features.md`.
