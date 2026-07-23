# AGENTS.md — `src/core/`

Xray **config and process adapter** layer. Do not confuse with `src/cores/` (binary update manager).

## Owns

- `ICoreAdapter` — core-specific config generation
- `XrayAdapter`, `XrayConfigBuilder` — build Xray JSON from profiles/routing/DNS
- `CoreManager` — spawn/validate/stop external core processes (`xray run -test` before start)

## Rules

- Config generation must stay offline-capable (no requirement that Xray is installed until start).
- Prefer small, testable changes; cover with `zarya_xray_config_test` / `scripts/run-xray-config-test.ps1`.
- Do not put GitHub download/install logic here — that belongs in `src/cores/`.
- Secrets in generated configs must not leak into logs; use redaction helpers when dumping.

Related: `src/runtime/xray/`, `docs/core-update-manager.md` (binary updates only).
