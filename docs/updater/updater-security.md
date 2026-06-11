# Updater security

## Rules

- Never install unsigned/unverified artifacts automatically
- Never update `zarya-helper` without stronger verification
- Never accept manifests over insecure transport for production (HTTPS required later)
- Manifest should be signed or fetched from a trusted release channel in future
- Checksum alone protects integrity if the manifest itself is trusted

## 0.32 behavior

- Download requires SHA-256 unless **Allow unsigned app update download** is enabled
- Install/replace is always disabled in beta
- Manifest URLs with credentials are redacted in diagnostics

## Future

- Signed manifests or detached signatures
- Code signing gate for production auto-update
