# macOS bundle

CMake builds `Zarya.app` when `APPLE` is true (`MACOSX_BUNDLE`).

Use `scripts/package-macos.sh` to stage a zip artifact.

Unsigned builds may show Gatekeeper warnings. Notarization is out of scope for 0.10.
