# Go / No-Go Checklist

Use before tagging 1.0 or a stable-channel release.

## Release metadata

- [ ] version correct
- [ ] release notes updated
- [ ] checksums generated
- [ ] artifact verification passed (`verify-release-artifacts.py --stable-release`)

## Stable features

- [ ] Xray system-proxy smoke passed
- [ ] profile import passed
- [ ] subscription update passed
- [ ] proxy restore passed
- [ ] backup/import passed
- [ ] diagnostics redaction passed

## Safety

- [ ] no critical bugs open in recommended path
- [ ] no high bugs in recommended path open
- [ ] experimental features gated on stable channel
- [ ] recovery docs accessible

## Documentation

- [ ] stable scope reviewed
- [ ] risk register reviewed
- [ ] known limitations updated

## Decision

- [ ] **Go**
- [ ] **No-go**
