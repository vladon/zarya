# Installer Security

## Threat model

An installer can place a privileged helper/service. This is more sensitive than extracting a portable archive.

## Principles

- install helper only from signed trusted builds
- never auto-install helper without user consent
- service must expose only local IPC
- service command set must stay allowlisted
- path restrictions remain required
- uninstall/recover must remove only Zarya-owned objects

## Signing

Production installer should be signed.

Unsigned installer should **not** install privileged helper by default.

## Future updater implication

App self-update must not replace helper without verification. Self-update is out of scope for 0.31.

## Data migration

Portable-to-installed migration must not delete source data or run without preview and confirmation.
