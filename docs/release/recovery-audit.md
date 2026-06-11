# Recovery Audit

## System proxy

- [ ] kill app while proxy enabled
- [ ] restart detects previous state
- [ ] restore succeeds
- [ ] diagnostics can be created

## Runtime

- [ ] Xray process killed externally
- [ ] UI detects stopped/failed state
- [ ] Stop is idempotent

## Import

- [ ] interrupted import leaves pre-import backup
- [ ] bad backup rejected

## Updater PoC

- [ ] failed update rollback message appears
- [ ] update-failed.json handled

## Experimental stale state

- [ ] stale kill switch marker visible even when experimental hidden
- [ ] helper recovery visible if stale state detected
