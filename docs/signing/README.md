# Zarya Signed Builds Plan

## Current state

0.27 adds signing-ready hooks but does not require real credentials.

## Goals

- reduce OS warnings
- improve artifact integrity
- prepare for auto-update trust model later

## Non-goals

- app self-update
- certificate procurement
- key management service implementation

## Platform summary

| Platform | Mechanism | 0.27 status |
|---|---|---|
| Windows | Authenticode | optional hook |
| macOS | Developer ID + notarization | optional hook |
| Linux | SHA256 + optional GPG/minisign | optional hook |

## Release levels

### Dev

Unsigned, local only.

### Beta

Unsigned or optionally signed. SHA256 required.

### Release candidate

Signed where credentials are available.

### Stable

Signing required for Windows/macOS.

## Documentation

- [Windows Authenticode](windows-authenticode.md)
- [macOS signing and notarization](macos-signing-notarization.md)
- [Linux artifact signing](linux-signing.md)
- [Key management](key-management.md)
- [Release verification](release-verification.md)

## Packaging flags

All platform package scripts default to unsigned builds (`--skip-signing`).

| Flag | Windows | macOS | Linux |
|------|---------|-------|-------|
| `--sign` / `-Sign` | Authenticode | codesign | enable signing hooks |
| `--skip-signing` | skip (default) | skip (default) | skip (default) |
| `--signing-identity` | cert thumbprint | Developer ID | — |
| `--timestamp-url` | RFC3161 URL | — | — |
| `--notarize` | — | notarytool | — |
| `--gpg-sign` | — | — | detached GPG |
| `--minisign` | — | — | minisign |

Verification: `python scripts/verify-release-artifacts.py --help`
