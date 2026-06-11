# macOS Privileged Helper Direction

## SMAppService

Apple's ServiceManagement API (`SMAppService`) is the modern path for LoginItems, LaunchAgents, and LaunchDaemons on macOS 13+.

## Signing/notarization requirements

Production helper installation requires:

- Developer ID signing
- notarization
- helper executable inside the app bundle
- ServiceManagement entitlements

## Why 0.28 does not install macOS helper

0.28 provides design documentation and detection only. No destructive install is performed.

## Future work

- bundle helper in `Contents/Library/LaunchServices` or app-managed SMAppService registration
- signed/notarized release pipeline integration

SMJobBless remains a compatibility note for older designs; new work should target SMAppService.
