# Zarya Helper Service

## Why helper service exists

TUN mode and kill switch need elevated operations.
The GUI should not run as Administrator/root.

## Current state

0.28 adds Windows service PoC and Linux/macOS design skeletons.

## Supported modes

- Manual helper
- Windows Service PoC
- Linux systemd design/skeleton
- macOS SMAppService design/skeleton

## Security model

- local IPC only
- token/session auth
- path restrictions
- helper command allowlist

See [helper-security-model.md](helper-security-model.md) and [helper-recovery.md](helper-recovery.md).
