# macOS Installer Strategy

## Distribution options

### DMG with .app

Recommended first production path.

Pros:

- common macOS distribution
- simple user model
- works for non-privileged app

Cons:

- privileged helper installation needs extra flow

### PKG

Needed if we install privileged helper/LaunchDaemon.

Pros:

- can install privileged components
- supports pre/postinstall scripts

Cons:

- higher risk; signing/notarization required

## Recommendation

1. **1.0:** signed/notarized `.app` in DMG for system-proxy mode.
2. **Later:** PKG or SMAppService-based privileged helper flow for TUN/helper.

## 0.31 skeleton

- `packaging/macos/dmg/create-dmg-notes.md`
- `packaging/macos/pkg/` — placeholder scripts only (no destructive install)
