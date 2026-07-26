Release builds may ship a bundled Xray executable in this directory so the app can start without downloading on first launch.

You can:
- leave the bundled binary as-is
- replace it via Tools → Core Manager (updates the managed install)
- point Settings → Xray executable at another binary (external path)

Expected executable:
- Windows: xray.exe
- macOS/Linux: xray

Geo Data Manager can place geoip.dat and geosite.dat in this directory (geo data is not bundled by default).
