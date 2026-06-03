# Zarya

Zarya is a cross-platform Qt 6 desktop client for managing proxy profiles and launching external proxy cores (Xray, sing-box). This repository contains milestone **0.2**: VLESS REALITY profiles, Xray config generation with validation, `vless://` import, and configurable local proxy ports.

## Requirements

- **CMake** 3.21 or newer
- **C++20** compiler (MSVC 2019+, GCC 10+, Clang 12+)
- **Qt 6.2+** with modules: Core, Gui, Widgets

No other third-party libraries are required for the application itself. **Xray** is optional and only needed when starting a profile.

## Build

### Windows (primary)

Requires **Visual Studio 2026** (or 2022+) with the **Desktop development with C++** workload, plus **Qt 6** built for MSVC (`msvc2022_64` kit — compatible with the VS 2026 toolset).

```powershell
# One-time: install Qt MSVC kit (if missing)
python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O C:\Qt

# Configure with VS 2026 generator
.\scripts\configure-msvc2026.ps1

# Build and run
cmake --build build --config Release --target zarya
.\build\Release\zarya.exe

# Config test
.\scripts\run-xray-config-test.ps1
```

### Static Release binary (no Qt DLLs)

Build static Qt once, then link `zarya` against it:

```powershell
.\scripts\build-qt-static-msvc2026.ps1    # installs to C:\Qt\Static\6.8.3\msvc2022_64
.\scripts\configure-msvc2026.ps1 -Static -Force
cmake --build build --config Release --target zarya
.\build\Release\zarya.exe                 # portable; Qt compiled in (/MT)
```

Or use preset `windows-msvc2026-static-release` with `QT_STATIC_DIR=C:/Qt/Static/6.8.3/msvc2022_64`.

Shared Qt remains the default for faster iteration; the unit test target uses shared/static Qt Core matching your prefix.

### macOS

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build
./build/zarya
```

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake --build build
./build/zarya
```

Adjust `CMAKE_PREFIX_PATH` to match your Qt 6 installation.

## Proxy core binaries

Place **Xray** next to the built `zarya` binary (or set an explicit path in **Settings**):

| Platform | Default path (if Settings empty) |
|----------|----------------------------------|
| Windows  | `./cores/xray/xray.exe` |
| Linux/macOS | `./cores/xray/xray` |

Example layout for local testing:

```
zarya/
  build/Release/
    zarya.exe
  cores/
    xray/
      xray.exe
```

In **Settings**, you can use a relative path such as `.\cores\xray\xray.exe` (from the working directory when you launch Zarya) or an absolute path.

The app **starts and runs without** Xray installed. Profile management, import, and config generation work offline. Starting a profile runs `xray run -test` first; if Xray is missing or validation fails, the core is not started and the log panel shows details.

## Configure Xray path

1. Open **File → Settings…**
2. Set **Xray executable** (Browse or paste path).
3. Set **Local SOCKS port** (default `10808`) and **Local HTTP port** (default `10809`).
4. Click **Save**.

Settings are stored with `QSettings` (organization **Zarya**).

## Import a VLESS REALITY link

1. **Profiles → Import VLESS link…** (or toolbar).
2. Paste one `vless://` URI per line, for example:

```
vless://UUID@host.example.com:443?type=tcp&security=reality&pbk=PUBLIC_KEY&fp=chrome&sni=example.com&sid=SHORT_ID&spx=%2F&flow=xtls-rprx-vision#Test%20Reality
```

3. Confirm import; profiles are added to the table and saved automatically.

Query parameters: `type` → network, `security`, `pbk` → public key, `fp` → fingerprint, `sni` → server name, `sid` → short ID, `spx` → spiderX, `flow`, `encryption` (default `none`). The URL fragment (`#…`) becomes the profile name.

## Data locations

Profiles are stored using Qt standard paths (`AppConfigLocation` / organization **Zarya**):

| OS      | Profiles file |
|---------|----------------|
| Windows | `%APPDATA%/Zarya/profiles.json` |
| macOS   | `~/Library/Application Support/Zarya/profiles.json` |
| Linux   | `~/.config/Zarya/profiles.json` |

Generated runtime configs:

- `…/Zarya/runtime/config-xray.json`

## Usage (0.2)

1. Launch **zarya**.
2. Configure **Xray** path in Settings if needed.
3. **Add** a VLESS REALITY profile manually, or **Import VLESS link…**.
4. **Save** writes `profiles.json`; **Load** reloads from disk.
5. Select a profile and click **Start**:
   - Config is generated and written to the runtime path.
   - `xray run -test -config …` runs; on failure, a dialog and log output appear.
   - On success, `xray run -config …` starts.
6. **Stop** terminates the process (terminate, wait 3s, then kill).
7. Core stdout/stderr appear in the log panel.

Local inbounds when Xray starts (from generated config, ports from Settings):

- SOCKS: `127.0.0.1:10808` (default)
- HTTP: `127.0.0.1:10809` (default)

Expected log sequence after **Start**:

```
Generating config…
Config path: …
Validating Xray config…
Validation OK
Starting Xray…
Xray started
SOCKS: 127.0.0.1:10808
HTTP: 127.0.0.1:10809
```

## Project layout

```
src/
  app/        Application entry, QApplication setup
  ui/         MainWindow, ProfileDialog, SettingsDialog, ImportVlessDialog
  domain/     Profile, validation, ProtocolType, CoreType
  core/       ICoreAdapter, XrayVlessGenerator, CoreManager
  import/     VlessUriParser
  storage/    ProfileStore, AppSettings, AppPaths
  platform/   Default core executable paths
```

## Current limitations

- **Xray**: VLESS with TLS or REALITY (TCP); `xtls-rprx-vision` flow supported.
- **sing-box**: adapter stub only; cannot start in 0.2.
- No system proxy, TUN, subscriptions, routing/DNS editors, delay tests, tray icon, or auto-update.
- No packaging/installer in this milestone.
- Milestone 0.1 `profiles.json` files still load; missing fields get safe defaults.

Run `zarya_xray_config_test` (or `.\scripts\run-xray-config-test.ps1`) to verify REALITY JSON generation.

## License

See [LICENSE](LICENSE).
