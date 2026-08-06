# AGENTS.md — `src/cores/`

**Core binary update manager** (download / verify / install / rollback of external sing-box; embedded Xray metadata is read-only). Not the Xray config adapter (`src/core/`).

## Owns

- Fetch sing-box release assets, checksum verification, install into `cores/`, rollback on failure
- UI/dialog wiring may live under `ui/`; keep download/verify logic here

## Rules

- Never treat this as Zarya **app** self-update — that is `src/updater/`.
- Verify checksums before install; fail closed on mismatch.
- Do not vendor core binaries in git; `cores/xray/` stores geo assets and `cores/sing-box/` stores the managed sing-box executable.
- Xray is embedded from `third_party/xray-core` and updated only with the Zarya app; never offer binary update/rollback/path actions for it.
- Respect network/error surfacing to the UI; no silent partial installs.

Docs: `docs/core-update-manager.md`, `docs/release-packaging.md`.
