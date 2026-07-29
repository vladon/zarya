# Known Issues for Zarya 1.5.0

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
- KDE integration requires KConfig 5 or 6 command-line tools and a usable
  session D-Bus. The UI reports system-proxy integration unavailable when
  either facility is missing.
