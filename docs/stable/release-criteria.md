# Zarya 1.0 Release Criteria

0.36.0-rc1 validates these criteria on the stable candidate path. See [docs/rc/rc-go-no-go.md](../rc/rc-go-no-go.md) and [docs/rc/rc-blockers.md](../rc/rc-blockers.md).

## Must pass

### Startup

- clean portable start works
- first-run wizard works
- app starts without cores installed

### Xray system-proxy mode

- install/detect Xray
- import profile
- validate config
- start/stop profile
- restore system proxy

### Profiles/subscriptions

- manual profile CRUD works
- subscription update works
- failed subscription update preserves old profiles

### Routing/DNS

- Proxy All validates
- Bypass LAN validates
- Secure Remote DNS validates
- missing geo data shows actionable warning

### Recovery

- unclean shutdown recovery works
- system proxy recovery works
- safe shutdown is idempotent

### Backup/diagnostics

- backup export/import works
- diagnostics bundle is redacted
- no helper token or raw runtime configs in diagnostics

### Packaging

- artifacts build
- checksums generated
- artifact verification passes
- no user data included in artifacts

## Must not happen

- system proxy left enabled after safe exit
- diagnostics leak raw profile links
- import corrupts existing config without pre-import backup
- app crashes on first launch
- stable build auto-starts experimental TUN
