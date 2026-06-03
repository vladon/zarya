# Zarya

Zarya is a cross-platform Qt 6 desktop client for managing proxy profiles and launching external proxy cores (Xray, sing-box). This repository contains milestone **0.1**: a walking skeleton with profile CRUD, JSON persistence, and basic core process management.

## Requirements

- **CMake** 3.21 or newer
- **C++20** compiler (MSVC 2019+, GCC 10+, Clang 12+)
- **Qt 6.2+** with modules: Core, Gui, Widgets

No other third-party libraries are required for this milestone.

## Build

### Windows (primary)

```powershell
# Ensure Qt 6 is on PATH, or set CMAKE_PREFIX_PATH to your Qt installation
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build --config Release
.\build\Release\zarya.exe
```

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

Place executables next to the built `zarya` binary (or in the working directory when running from the build tree):

| Platform | Xray path | sing-box path |
|----------|-----------|---------------|
| Windows  | `./cores/xray/xray.exe` | `./cores/sing-box/sing-box.exe` |
| Linux/macOS | `./cores/xray/xray` | `./cores/sing-box/sing-box` |

Example layout after build:

```
build/Release/
  zarya.exe
  cores/
    xray/
      xray.exe
```

The app **starts and runs without** these binaries. Profile management works offline. Starting a core shows a clear error if the executable is missing.

## Data locations

Profiles are stored using Qt standard paths (`AppConfigLocation` / organization **Zarya**):

| OS      | Profiles file |
|---------|----------------|
| Windows | `%APPDATA%/Zarya/profiles.json` (typically `%LOCALAPPDATA%` varies; Qt uses roaming AppData config) |
| macOS   | `~/Library/Application Support/Zarya/profiles.json` |
| Linux   | `~/.config/Zarya/profiles.json` |

Generated runtime configs:

- `…/Zarya/runtime/config-xray.json`
- `…/Zarya/runtime/config-singbox.json`

## Usage (0.1)

1. Launch **zarya**.
2. **Add** profiles via toolbar or menu; edit fields in the dialog.
3. **Save** writes `profiles.json`; **Load** reloads from disk.
4. Select a profile and click **Start** to generate an Xray config (VLESS only) and launch the core.
5. **Stop** terminates the process (graceful terminate, then kill after timeout).
6. Core stdout/stderr appear in the log panel.

Local inbounds when Xray starts (generated config):

- SOCKS: `127.0.0.1:10808`
- HTTP: `127.0.0.1:10809`

## Project layout

```
src/
  app/        Application entry, QApplication setup
  ui/         MainWindow, ProfileDialog, table model
  domain/     Profile, ProtocolType, CoreType
  core/       ICoreAdapter, Xray/SingBox adapters, CoreManager
  storage/    ProfileStore, AppPaths
  platform/   Default core executable paths
```

## Current limitations

- **Xray**: minimal VLESS outbound only; other protocols return a clear “not implemented” message.
- **sing-box**: adapter stub only; config generation not implemented.
- No system proxy, TUN, subscriptions, routing/DNS editors, delay tests, tray icon, or auto-update.
- Core paths are fixed relative to the application directory (`./cores/…`).
- No packaging/installer in this milestone.

## License

See [LICENSE](LICENSE).
