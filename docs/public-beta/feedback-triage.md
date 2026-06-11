# Public Beta Feedback Triage

## Intake

Every bug should have:

- Zarya version
- platform
- runtime mode
- steps to reproduce
- expected result
- actual result
- diagnostics bundle if possible

## First response

1. Check if diagnostics bundle is attached.
2. Check runtime mode.
3. Check whether issue affects recommended path or experimental path.
4. Assign severity.
5. Assign area label.

## Severity rules

### Critical

- app does not start
- system proxy cannot be restored
- kill switch cannot be disabled
- helper accepts invalid token
- diagnostics leaks secrets
- backup/import corrupts data

### High

- valid Xray profile cannot start
- Core Manager cannot install Xray
- subscription update destroys profiles
- packaging artifact unusable
- first-run wizard blocks setup

### Medium

- confusing error
- non-critical crash in experimental path
- missing status refresh

### Low

- copy/layout/localization issue

See also [beta-blockers.md](beta-blockers.md) for release gating.
