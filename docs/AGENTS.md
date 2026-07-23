# AGENTS.md — `docs/`

Design and release documentation. Prefer updating the doc that owns the topic rather than duplicating in README.

## Layout

| Area | Path |
|------|------|
| Stable 1.0 scope / gating / criteria | `stable/` |
| Public beta / experimental UX | `public-beta/` |
| RC / release process | `rc/`, `release/` |
| Helper service | `service/` |
| Installers / migration | `installer/` |
| App updater | `updater/` |
| Signing | `signing/` |
| Feature designs | `tun-design.md`, `kill-switch-design.md`, `privileged-helper-design.md`, … |

## Rules

- Code behavior and `docs/stable/` win over older README “limitations” bullets when they conflict.
- Experimental feature docs should mention gating (`feature-gating.md`).
- Release notes live under `release-notes/` / `release-drafts/`; do not invent version numbers — match `cmake/ZaryaVersion.cmake`.
- Keep security/privacy claims aligned with `security-model.md` and redaction behavior in code.
- Prefer short, checklist-oriented release docs; link out instead of copying large matrices.
