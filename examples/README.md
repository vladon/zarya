# Zarya config examples

Reference artifacts for Xray VLESS REALITY config generation.

## Files

| File | Purpose |
|------|---------|
| `profiles-vless-reality.sample.json` | Profile store snippet (loads on v1 schema; REALITY fields optional for older profiles) |
| `xray-vless-reality.sample.json` | Expected full Xray JSON for the sample profile |

## Regenerate / verify

Build the config check tool:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt6>
cmake --build build --target zarya_xray_config_test
./build/zarya_xray_config_test
```

The tool uses `XrayConfigTestHelpers` to generate config from the sample profile and compares key structure against `xray-vless-reality.sample.json`.

## REALITY profile fields

| Profile field | Xray `realitySettings` |
|---------------|------------------------|
| `serverName` (or legacy `sni`) | `serverName` |
| `publicKey` | `publicKey` |
| `shortId` | `shortId` |
| `fingerprint` | `fingerprint` (default `chrome`) |
| `spiderX` | `spiderX` (default `/`) |
| `flow` | VLESS user `flow` (default `xtls-rprx-vision` when empty for REALITY) |

Legacy profiles without the new JSON keys continue to load; TLS profiles still use `tlsSettings.serverName` from `serverName` or `sni`.
