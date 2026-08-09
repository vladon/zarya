# Zarya Core Runtime Manager

## Runtime distributions

- **Xray** — embedded system-proxy runtime in the GUI.
- **sing-box** — embedded experimental TUN runtime in the privileged `zarya-helper`.

Core Manager shows **Built into Zarya**, the core and ABI versions, and runtime load status for both cores. Download, update, rollback, folder and executable-path actions are unavailable: core updates are delivered only through the Zarya app update.

`cores/xray/` contains geo assets and matcher cache. sing-box rule-set artifacts are compiled by the helper from in-memory configuration; no runtime configuration containing credentials is written to disk.

## Troubleshooting

- **Embedded bridge unavailable** — repair or reinstall Zarya. Do not install a standalone core executable.
- **TUN cannot start** — inspect the helper diagnostics, then verify required privileges and driver state.
- **Geo assets are missing** — restore the packaged geo data or repair the application.

## Portable mode

Portable mode keeps application data under `./data` and geo assets under `./cores/`; both cores still come from the application package.