# Zarya Diagnostics Bundle

## Purpose

A diagnostics bundle is a **redacted troubleshooting archive** for support and debugging. It is **not** a backup and cannot restore your configuration.

Use it when investigating:

- Core startup failures
- sing-box validation errors
- Routing/DNS issues
- Helper connectivity problems
- Kill switch recovery issues
- Geo data or rule-set problems

Create via **Help → Create Diagnostics Bundle…**

## What is included

Default bundle (`.zarya-diagnostics.zip`):

- `manifest.json` with format version, categories, SHA-256 checksums
- `diagnostics/` — app, platform, paths, runtime, helper, proxy, kill switch, cores, routing, DNS, geo, rule-sets, validation, errors
- `configs/` — redacted Xray/sing-box config **previews**
- `logs/app.redacted.log` — recent application log lines (redacted)
- `raw/README.txt` — explains intentional omissions

## What is never included

- `helper.token`
- Raw runtime generated configs with secrets
- Raw subscription URLs or share links
- Credentials (UUID, password, keys)
- Kill switch marker raw dump
- Packet captures or traffic logs
- Core binaries

## Redaction modes

| Mode | Behavior |
|------|----------|
| **Strict** (default) | Redact credentials, hosts, URLs, usernames in paths |
| **Basic** | Redact credentials; keep hosts and ports |

Always review the bundle before sharing.

## How to create a bundle

1. **Help → Create Diagnostics Bundle…**
2. Choose redaction mode and options
3. Optional: **Preview** to see files and warnings
4. **Create Bundle**
5. Review output path; open folder and inspect before sending

Options:

- **Run config validation** — runs Xray/sing-box check on selected profile (default on)
- **Extended logs** — up to 5000 lines instead of 1000
- **Machine paths** — include more path detail (still redacted in strict mode)

## How to review before sharing

1. Extract the ZIP
2. Search for secrets: `vless://`, `password`, `uuid`, `helper.token`
3. Confirm only `<redacted>` placeholders appear for sensitive fields
4. Check `diagnostics/redaction-report.json`

## Troubleshooting

| Issue | Notes |
|-------|-------|
| Helper shows disconnected | Bundle still created; see `helper-status.json` |
| Validation failed | Warning in manifest; see `validation-summary.json` |
| No profile selected | Validation skipped; config previews note reason |
| Collector warning | Non-critical; bundle completes with manifest warning |

## Difference from backup

| | Backup | Diagnostics |
|---|--------|-------------|
| Goal | Restore configuration | Troubleshoot issues |
| Secrets | Full backup may include real data | Always redacted |
| Restore | Yes | No |
