# Zarya 1.5.0 Platform Test Matrix

Minimum environments for stable validation. TUN/kill-switch tests are separate (experimental).

## Matrix

| Environment | Clean start | Core install | Import | Proxy apply | Stop/exit restore | Diagnostics | Backup |
|-------------|-------------|--------------|--------|-------------|-------------------|-------------|--------|
| Windows 10 x64 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| Windows 11 x64 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| macOS Apple Silicon | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| macOS Intel (if available) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| Ubuntu 24.04 GNOME | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| KDE Plasma 6 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| KDE Plasma 5 fallback | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |

## Automated smoke (all platforms in CI)

```bash
python scripts/run-smoke-tests.py --source-tree .
cmake --build build --target zarya_smoke_test
python scripts/run-smoke-tests.py --build-dir build --skip-cpp  # artifact optional
```

Windows packaging:

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist
python .\scripts\run-smoke-tests.py --artifact .\dist\Zarya-1.5.0-windows-x64-portable.zip --build-dir .\build
```

## Proxy restore verification

**Windows:** `Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'`

**macOS:** `networksetup -getwebproxy "Wi-Fi"` / `networksetup -getsecurewebproxy "Wi-Fi"`

**GNOME:** `gsettings get org.gnome.system.proxy mode`

**KDE/Plasma:** capture every managed key with `kreadconfig6 --file kioslaverc
--group "Proxy Settings" --key KEY` (use `kreadconfig5` on Plasma 5). Exercise
apply, normal stop, tray exit, and startup recovery. Confirm the HTTP/HTTPS,
no-proxy, PAC, reversed-exception, and proxy-type values and key presence match
the snapshot after each restore. Record the desktop version, KConfig tool
version, commands, and result in the release checklist.

## Secret audit (diagnostics / redacted backup)

After creating bundles, search extracted contents:

```bash
grep -R "vless://" .
grep -R "trojan://" .
grep -R "vmess://" .
grep -R "ss://" .
grep -R "helper.token" .
```

Redacted diagnostics must not contain raw secrets. Full backups may contain secrets by design.
Release archives must also pass:

```bash
python scripts/audit-release-artifact.py --artifact dist/Zarya-1.5.0-linux-x64.tar.gz
```

## Experimental matrix (optional)

| Environment | TUN start | Helper connect | Kill switch enable/disable |
|-------------|-----------|----------------|----------------------------|
| Windows 11 x64 (admin) | [ ] | [ ] | [ ] |
| Ubuntu 24.04 | [ ] | [ ] | [ ] |
| macOS | [ ] | [ ] | n/a (unsupported) |
