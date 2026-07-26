# Bundled geo data pins

Release packaging downloads **pinned** `geoip.dat` / `geosite.dat` from runetfreedom into `cores/xray/` (next to the Xray executable) so bypass routing works offline on first launch.

- Do **not** commit `.dat` files to git.
- Update `runetfreedom-pin.json` (tag + SHA-256) when bumping the bundled geo set.
- Override cache with `ZARYA_GEODATA_BUNDLE_CACHE`.
- Disable with `ZARYA_BUNDLE_GEODATA=0` or package script `-SkipBundleGeodata` / `--skip-bundle-geodata`.

Users can still replace files via **Tools → Geo Data Manager** (any built-in source).
