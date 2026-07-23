# AGENTS.md — `src/features/`

Release-channel feature gating for experimental surfaces.

## Owns

- `FeatureGate` — visibility / enablement checks
- `FeaturePolicy` — defaults per channel (`stable`, `rc`, `beta`, `dev`)
- Settings keys: `release/channel`, `release/showExperimentalFeatures`

## Rules

- On stable (default): hide TUN / helper / kill switch / experimental Tools; effective runtime → Xray system-proxy.
- Do not delete user TUN settings when gating; preserve configuration.
- Recovery exception: kill switch and system proxy restore stay available when UI is hidden.
- New experimental features need a `FeatureId` + policy entry + docs under `docs/stable/` / `docs/public-beta/`.
- Cover gating with `zarya_stable_hardening_test` when changing policy.

Docs: `docs/stable/feature-gating.md`, `docs/stable/stable-scope.md`.
