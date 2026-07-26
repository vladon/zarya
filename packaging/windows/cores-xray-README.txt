Release builds may ship a bundled Xray executable in this directory so the app can start without downloading on first launch.

You can:
- leave the bundled binary as-is
- replace it via Tools → Core Manager (updates the managed install)
- point Settings → Xray executable at another binary (external path)

Expected executable:
- Windows: xray.exe
- macOS/Linux: xray

Release builds may also ship pinned runetfreedom geoip.dat and geosite.dat here so bypass routing works offline. Tools → Geo Data Manager can still update or switch sources.
