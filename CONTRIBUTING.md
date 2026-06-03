# Contributing to Zarya

## Trunk-based development

`main` is the trunk. It should always stay buildable and releasable.

1. **Branch** — Create a short-lived branch from the latest `main`:
   ```bash
   git fetch origin
   git checkout main
   git pull origin main
   git checkout -b feat/short-description
   ```
2. **Work** — Make small, focused commits on your branch.
3. **Push** — Push the branch (never push directly to `main`):
   ```bash
   git push -u origin feat/short-description
   ```
4. **Pull request** — Open a PR into `main` on GitHub.
5. **Review & merge** — After checks pass, merge the PR (squash or merge commit per team preference), then delete the branch.
6. **Sync** — Update your local trunk:
   ```bash
   git checkout main
   git pull origin main
   ```

Do not commit long-lived feature branches. Keep branches alive only for the current change.

## Branch naming

- `feat/` — new functionality
- `fix/` — bug fixes
- `chore/` — tooling, docs, refactors without behavior change

## Pull requests

- Target branch: `main`
- One logical change per PR when possible
- Include a brief test plan in the PR description
