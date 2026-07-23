# AGENTS.md — `src/helper/`

Privileged helper executable (`zarya-helper`) for elevated operations (TUN, kill switch). Related: `src/helperclient/`, `src/ipc/`, `src/service/`.

## Boundaries

- GUI stays unprivileged; talk to helper via `helperclient` + `ipc` (local socket/pipe, auth token).
- Helper performs privileged work only; path/command allowlists and auth are security-sensitive — tighten, never loosen casually.
- OS service install/lifecycle: `src/service/` (Windows/Linux/macOS/stub).

## Rules

- Experimental on stable builds — visibility/enablement via `FeatureGate`.
- Kill switch / system proxy **recovery** must remain available even when experimental UI is hidden.
- Do not run the main `zarya` GUI as admin to “fix” privilege issues.
- Prefer fail-safe teardown (restore networking) on helper crash/disconnect.

Docs: `docs/privileged-helper-design.md`, `docs/service/`, `docs/security-model.md`.
