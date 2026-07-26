# Bundled core pins

Release packaging may download a **pinned** Xray binary into `cores/xray/` so first launch works offline.

- Do **not** commit core binaries to git.
- Update `xray-pin.json` (tag + per-asset SHA-256) when bumping the bundled Xray version.
- Override cache with `ZARYA_XRAY_BUNDLE_CACHE`.
- Disable bundling with `ZARYA_BUNDLE_XRAY=0` or package script `-SkipBundleXray` / `--skip-bundle-xray`.

sing-box is not bundled (experimental TUN only).
