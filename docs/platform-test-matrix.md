# Zarya Platform Test Matrix (0.26 Beta)

Minimum environments for manual beta validation. TUN/kill-switch tests are separate (experimental).

## Matrix

| Environment | Clean start | Core Manager install Xray | Import VLESS | Start/stop | Proxy restore | Diagnostics | Backup export |
|-------------|-------------|----------------------------|--------------|------------|---------------|-------------|---------------|
| Windows 10 x64 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| Windows 11 x64 | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| macOS Apple Silicon | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| macOS Intel (if available) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| Ubuntu 24.04 GNOME | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |
| KDE Plasma Linux (if available) | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] |

## Automated smoke (all platforms in CI)

```bash
python scripts/run-smoke-tests.py --source-tree .
cmake --build build --target zarya_smoke_test
python scripts/run-smoke-tests.py --build-dir build --skip-cpp  # artifact optional
```

Windows packaging:

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist
python .\scripts\run-smoke-tests.py --artifact .\dist\Zarya-0.26.0-beta-windows-x64-portable.zip --build-dir .\build
```

## Proxy restore verification

**Windows:** `Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'`

**macOS:** `networksetup -getwebproxy "Wi-Fi"` / `networksetup -getsecurewebproxy "Wi-Fi"`

**GNOME:** `gsettings get org.gnome.system.proxy mode`

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

## Experimental matrix (optional)

| Environment | TUN start | Helper connect | Kill switch enable/disable |
|-------------|-----------|----------------|----------------------------|
| Windows 11 x64 (admin) | [ ] | [ ] | [ ] |
| Ubuntu 24.04 | [ ] | [ ] | [ ] |
| macOS | [ ] | [ ] | n/a (unsupported) |
