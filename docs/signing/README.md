# Zarya Signed Builds Plan

## Current state

Windows release signing is prepared for SignPath Foundation origin-verified builds.
It remains disabled until the project is approved and the protected release-signing
environment is configured. Local certificate hooks remain available for maintainers.

## Goals

- reduce OS warnings
- improve artifact integrity
- prepare for auto-update trust model later

## Non-goals

- app self-update
- certificate procurement
- key management service implementation

## Platform summary

| Platform | Mechanism | Current status |
|---|---|---|
| Windows | SignPath Foundation Authenticode | prepared; activation pending |
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
- [Code signing policy](code-signing-policy.md)
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

## SignPath activation

The release workflow uses the checked-in
`.signpath/artifact-configuration.xml` and is enabled only when the repository
variable `SIGNPATH_ENABLED` is `true`. Configure these values after SignPath
Foundation approves the project:

- environment: `release-signing` with required reviewer approval
- secret: `SIGNPATH_API_TOKEN`
- variables: `SIGNPATH_ORGANIZATION_ID`, `SIGNPATH_PROJECT_SLUG`,
  `SIGNPATH_SIGNING_POLICY_SLUG`, `SIGNPATH_ARTIFACT_CONFIGURATION_SLUG`

The unsigned staging artifact is retained for one day and is never uploaded as a
release artifact when SignPath signing is enabled. The signed result is finalized
and checked with `--require-signed` before upload.
