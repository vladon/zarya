# Stable Regression Matrix

Recommended-path failures block 1.0. Experimental-path failures do not block stable unless they affect the recommended path.

## Windows 11 x64

- [ ] fresh start
- [ ] install Xray
- [ ] import profile
- [ ] start/stop
- [ ] system proxy restore
- [ ] subscription update
- [ ] backup export/import
- [ ] diagnostics redaction

## Windows 10 x64

Same as Windows 11 x64.

## macOS arm64

Same checks using `networksetup` system proxy.

## macOS x64

Same if hardware available.

## Ubuntu 24.04 GNOME

Same checks using gsettings proxy.

## KDE Linux

System proxy unsupported/partial warning must be clear. Recommended path smoke should document limitations.

---

# Experimental Regression Matrix

Separate from stable gate. Failures here do not block 1.0 if stable gating works.

- [ ] TUN via sing-box (beta channel, experimental enabled)
- [ ] zarya-helper connect
- [ ] Linux nftables kill switch PoC
- [ ] Windows WFP kill switch PoC
