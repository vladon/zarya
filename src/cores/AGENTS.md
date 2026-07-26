# AGENTS.md — `src/cores/`

**Core binary update manager** (download / verify / install / rollback of Xray and sing-box from GitHub). Not the Xray config adapter (`src/core/`).

## Owns

- Fetch release assets, checksum verification, install into `cores/`, rollback on failure
- UI/dialog wiring may live under `ui/`; keep download/verify logic here

## Rules

- Never treat this as Zarya **app** self-update — that is `src/updater/`.
- Verify checksums before install; fail closed on mismatch.
- Do not vendor core binaries in git; runtime layout is `cores/xray/`, `cores/sing-box/`.
- Release packaging may **seed** a pinned Xray into `cores/xray/` (`packaging/cores/xray-pin.json`); that is bootstrap only — Core Manager updates still replace the managed install.
- Respect network/error surfacing to the UI; no silent partial installs.

Docs: `docs/core-update-manager.md`, `docs/release-packaging.md`.
