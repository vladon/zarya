# macOS DMG notes (0.31 skeleton)

## Target

Signed/notarized `Zarya.app` inside a drag-to-Applications DMG.

## Steps (future)

1. Build `Zarya.app` bundle with helper, translations, docs.
2. Codesign app + helper with hardened runtime.
3. Notarize and staple.
4. Create DMG with `create-dmg` or `hdiutil`.
5. Ship SHA256 checksum alongside DMG.

## 0.31

Documentation only. No automated DMG build in CI yet.
