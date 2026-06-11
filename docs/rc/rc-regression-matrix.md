# Zarya 0.36 RC Regression Matrix

## Windows 11 x64 — primary

- [ ] fresh portable start
- [ ] first-run wizard
- [ ] install Xray via Core Manager
- [ ] import VLESS link
- [ ] start Xray
- [ ] system proxy enabled
- [ ] stop restores proxy
- [ ] subscription update
- [ ] node TCP test
- [ ] real delay test
- [ ] backup export/import
- [ ] diagnostics bundle redacted
- [ ] tray close/restore/exit
- [ ] updater install disabled by default

## Windows 10 x64

Same essential path as Windows 11 x64.

## macOS arm64

- [ ] app starts
- [ ] Xray install/detect
- [ ] networksetup proxy apply/restore
- [ ] diagnostics

## Linux Ubuntu GNOME

- [ ] app starts
- [ ] Xray install/detect
- [ ] gsettings proxy apply/restore
- [ ] diagnostics

## Linux KDE

- [ ] system proxy unsupported/partial warning
- [ ] app does not pretend proxy is enabled

## Experimental matrix (non-blocking)

Run separately when experimental features are explicitly enabled:

- [ ] TUN start/stop
- [ ] helper service controls
- [ ] kill switch enable/disable/recovery
- [ ] MSI PoC install (no helper service by default)
- [ ] app updater install PoC (dev/beta with flag enabled)
