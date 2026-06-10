# Signing Key Management

## Principles

- private keys must not be committed
- CI secrets must be scoped
- signing credentials should be unavailable to pull requests
- release signing should be gated/manual
- timestamping should be used where supported

## Windows

Prefer hardware-backed certificate or secure CI secret integration.

Environment variable for local/CI signing: `ZARYA_WINDOWS_CERT_THUMBPRINT`.

## macOS

Use Apple Developer ID certificate in secure keychain or CI signing identity.

Notarization credentials (`APPLE_ID`, team ID, app-specific password, or API key) must never be stored in the repository.

## Linux

GPG/minisign private key should be offline or protected.

## Future auto-update implication

The updater must trust a stable signing identity or public key.

## CI gate

Signing runs only when:

- the workflow is triggered by a release tag, and
- `SIGNING_ENABLED` repository secret is `true`

Pull request workflows never receive signing secrets.
