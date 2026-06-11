# WiX installer skeleton (0.31)

Planning skeleton only. **Not built by default.**

## Files

| File | Purpose |
|------|---------|
| `Variables.wxi` | Version, paths, upgrade GUID placeholders |
| `Product.wxs` | Product/package definition |
| `Zarya.wxs` | Application components |
| `Service.wxs` | Optional ZaryaHelper service (future) |

## Build (future)

Requires WiX Toolset v4+ and signed binaries. Not wired into CMake in 0.31.

## Components

- `ZaryaAppComponents` — `Zarya.exe`, `zarya-helper.exe`, translations, docs
- `ZaryaHelperServiceComponents` — service install (disabled in skeleton)
