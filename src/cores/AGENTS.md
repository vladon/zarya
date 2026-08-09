# AGENTS.md — `src/cores/`

**Core runtime metadata** for the embedded Xray and sing-box distributions. This is not the Xray config adapter (`src/core/`).

## Owns

- Read-only core metadata displayed by Core Manager
- Version, ABI and load-status reporting for embedded runtimes

## Rules

- Never treat this as Zarya **app** self-update — that is `src/updater/`.
- Do not vendor core binaries in git. `cores/xray/` stores geo assets; sing-box rule sets are helper-generated cache artifacts.
- Both Xray and sing-box are embedded and updated only through the Zarya app; never offer binary download, update, rollback or executable-path actions for either core.
- Keep GUI unprivileged: sing-box TUN runtime work belongs to `zarya-helper`.
- Respect network/error surfacing to the UI; never expose secrets in diagnostics.

Docs: `docs/core-update-manager.md`, `docs/release-packaging.md`.