# Security Model

## GUI process

The main `zarya` process runs as the logged-in user. It manages profiles, settings, UI, and orchestrates proxy cores. It should not require administrator rights for system-proxy mode.

## Helper process

`zarya-helper` runs elevated on platforms that require it (Linux TUN/kill switch, Windows WFP). It exposes a local IPC API for TUN and kill-switch operations only.

The helper accepts only:

- Runtime configs under the allowed runtime directory
- sing-box binaries under the allowed core directory (default `cores/sing-box/`, or the parent directory of a user-configured sing-box path in Settings)

## Local IPC

GUI ↔ helper communication uses a local socket/named pipe with a token file stored in the runtime data directory. The token is not included in diagnostics exports by default.

## Token file

The helper token authenticates IPC clients. Treat the runtime data directory as sensitive on multi-user systems.

## What Zarya does not protect against

- Malware running as the same user
- Compromised proxy servers or subscription providers
- DNS leaks outside configured profiles
- Traffic outside the configured runtime mode (non-proxied apps, misconfigured routes)

## Secret handling

Profile credentials live in local JSON stores under the user data directory. Config previews and exports may contain secrets; diagnostics and backups redact them by default.

## Diagnostics redaction

Diagnostics bundles use `DiagnosticsRedactor` / `ConfigRedactor` with strict mode by default. Share diagnostics bundles only after reviewing redaction warnings in the export dialog.
