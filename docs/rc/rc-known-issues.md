# Known Issues for 0.36 RC (historical)

> **Note:** Zarya **1.0.0** is the current stable release. See [docs/release/known-issues.md](../release/known-issues.md) for 1.0 stable known issues.

## Experimental features

- TUN/helper/kill switch are not part of stable support.
- macOS kill switch is not implemented.
- Windows MSI is a PoC.
- App self-update installation is disabled by default.

## Packaging

- Builds may be unsigned.
- Portable ZIP is the recommended artifact.

## System proxy

- System proxy mode is not VPN mode.
- Some applications may ignore OS proxy settings.
