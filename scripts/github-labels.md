# GitHub labels for Zarya beta triage

Label definitions live in [`.github/labels.yml`](../.github/labels.yml).

## Create labels manually

GitHub → **Settings** → **Labels** → **New label**, or use the GitHub CLI:

```bash
gh label create "severity:critical" --color b60205 --description "Data loss, app cannot start, networking cannot recover, secret leak"
gh label create "severity:high" --color d93f0b --description "Major feature broken, beta-blocking"
gh label create "severity:medium" --color fbca04 --description "Important but not beta-blocking"
gh label create "severity:low" --color 0e8a16 --description "Minor issue"
gh label create "area:runtime" --color 1d76db
gh label create "area:system-proxy" --color 1d76db
gh label create "area:tun" --color 5319e7
gh label create "area:helper" --color 5319e7
gh label create "area:kill-switch" --color 5319e7
gh label create "area:packaging" --color c5def5
gh label create "area:diagnostics" --color c5def5
gh label create "area:backup" --color c5def5
gh label create "area:localization" --color c5def5
gh label create "needs:diagnostics" --color fef2c0
gh label create "needs:repro" --color fef2c0
gh label create "beta-blocker" --color b60205
gh label create "experimental" --color 5319e7
```

## Sync from labels.yml (optional)

If you use a label-sync workflow or `gh` scripting, import from `.github/labels.yml` and keep issue templates aligned with [docs/public-beta/feedback-triage.md](../docs/public-beta/feedback-triage.md).
