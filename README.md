# Zarya

Zarya is a cross-platform Qt 6 desktop client for managing proxy profiles and launching external proxy cores (Xray, sing-box). Milestones 0.1–0.27: profiles, subscriptions, Xray, routing, geo data, DNS, system proxy, experimental TUN, sing-box rule sets, core update manager, backup import/export, diagnostics bundle, beta hardening, privileged helper, experimental kill switch (Linux nft / Windows WFP PoC), tray, autostart, English/Russian UI, release packaging, **0.26.0-beta** bugfix pass, and **0.27.0-beta** signing-ready packaging hooks (optional Authenticode/codesign/GPG, verification scripts, signing docs).

Zarya supports **English** and **Russian** UI. Change language in **Settings → General → Language** (restart required for full effect). See [docs/localization.md](docs/localization.md).

## Requirements

- **CMake** 3.21 or newer
- **C++20** compiler (MSVC 2019+, GCC 10+, Clang 12+)
- **Qt 6.2+** with modules: Core, Gui, Widgets, Network

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

# Tests
.\scripts\run-xray-config-test.ps1
.\build\Release\zarya_subscription_test.exe
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

## Quick start

1. Open **Tools → Core Manager** and install Xray.
2. Import a profile link (**File → Import Profile Links…**) or add a subscription.
3. Choose **Routing: Bypass LAN** (default) in the setup wizard or **Tools → Routing Profiles**.
4. Choose **DNS: System DNS** (default).
5. Click **Start**.

## First-run setup

Zarya shows a setup wizard on first launch. You can reopen it from **Help → Run Setup Wizard**.

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

## Subscriptions

Add subscription URLs under **Subscriptions → Manage**. Zarya downloads the list and imports share links into profiles grouped by subscription.

**Supported subscription formats:**

- Plain UTF-8 text — one share link per line
- Base64-encoded text of the same link list
- Share links: `vless://`, `vmess://`, `trojan://`, `ss://`

**Unsupported:** Clash YAML, sing-box JSON subscriptions, provider metadata blocks.

**Update behavior:**

- **Update Selected** (profile filter set to a subscription) or **Update All**
- Manual profiles are never changed or deleted
- Nodes removed from the remote list are marked `[missing]` (soft-delete), not erased
- Failed updates keep existing profiles unchanged

**Runnable through Xray** (when fields are complete): VLESS (REALITY/TLS/none), VMess (TCP/WS + TLS), Trojan (TCP/WS + TLS), Shadowsocks (no plugin). Profiles with unsupported features (e.g. SS plugin) stay in the table with an import note.

**Local test server:**

```powershell
cd examples
python -m http.server 8080
# Add subscription URL: http://127.0.0.1:8080/sub-plain.txt
```

Example plain subscription: [examples/sub-plain.txt](examples/sub-plain.txt)

Data files:

- `…/Zarya/subscriptions.json`
- `…/Zarya/profiles.json` (includes `sourceType`, `subscriptionId`, `sourceKey`)

## Windows system proxy

On **Windows**, Zarya can set the system HTTP/HTTPS proxy to the local Xray HTTP inbound (`127.0.0.1:<httpPort>`, default **10809**).

- **Settings → Windows system proxy**: enable proxy automatically when a profile starts, and restore previous settings on stop/exit (both on by default on Windows).
- **Tools → Enable System Proxy** / **Restore Previous Proxy** for manual control (enable requires a running core).
- Before enabling, Zarya saves your current WinINet proxy settings (`ProxyEnable`, `ProxyServer`, `ProxyOverride`, `AutoDetect`, `AutoConfigURL`) and restores them on **Stop** or application exit.
- Registry path: `HKCU\Software\Microsoft\Windows\CurrentVersion\Internet Settings`

**Notes:**

- Many browsers respect Windows system proxy; some apps ignore it.
- CLI tools often need explicit `HTTP_PROXY` / `HTTPS_PROXY` and are out of scope.
- **macOS / Linux**: system proxy is not implemented yet (stub only; no registry changes).

Verify proxy state in PowerShell:

```powershell
Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' |
  Select-Object ProxyEnable, ProxyServer, ProxyOverride, AutoDetect, AutoConfigURL
```

When Zarya enables proxy, expect roughly:

```
ProxyEnable = 1
ProxyServer = http=127.0.0.1:10809;https=127.0.0.1:10809
ProxyOverride = <local>
```

## Desktop behavior

- Closing the main window can **hide Zarya to the system tray** instead of quitting (default on Windows when a tray is available).
- Use **tray → Exit** or **File → Exit** to fully quit.
- On full exit, Zarya stops the core, cancels tests, and attempts to restore Windows system proxy settings.
- **Double-click** the tray icon to show or hide the main window.
- Some Linux desktops do not provide a system tray; close-to-tray is disabled automatically there.
- Change behavior in **Settings → Desktop behavior**.

## Node testing

Test profiles from the **Test** menu or toolbar:

- **Test Selected** / **Test All** — TCP reachability, then real HTTP delay (default).
- **Test TCP Selected** — connect to `host:port` only (no Xray).
- **Test Delay Selected** — temporary Xray on free local ports, HTTP request through the local proxy.
- **Cancel Tests** — stop queued/running tests.

The profile table shows **TCP**, **Delay**, **Test Status**, and **Last Tested**. Results are saved in `profiles.json`.

- **TCP test** checks server `host:port` reachability only; it does not validate the proxy protocol.
- **Real delay** starts a separate temporary Xray process (not the running core) and measures HTTP time to the test URL (default `https://www.google.com/generate_204`).
- **Real delay** uses the same Xray config generation as **Start**; unsupported variants show **Unsupported** with a reason.
- Change the test URL, timeouts, and max concurrent tests in **Settings → Testing**.
- Batch tests respect **max concurrent tests** (default 3); several temporary Xray processes may run at once.

Run `zarya_testing_test` for unit checks of TCP ping and port allocation.

## Routing profiles

Choose how traffic is routed through Xray in **Tools → Routing Profiles…** or **Settings → Routing**.

Built-in modes:

- **Proxy All** — all traffic uses the proxy outbound (default catch-all rule).
- **Bypass LAN** — `geosite:private` and `geoip:private` go **direct**.
- **Bypass RU** — `geosite:ru` and `geoip:ru` go **direct**.
- **Bypass LAN + RU** — combines private and RU bypass rules.
- **Custom** — edit your own direct/proxy/block domain, IP, port, and protocol rules.

Rules are translated into Xray `routing.rules` with `outboundTag` set to `proxy`, `direct`, or `block`. Block rules are ordered before direct, then proxy; a final catch-all sends remaining traffic to `proxy`.

- `geosite:…` and `geoip:…` require compatible `geosite.dat` / `geoip.dat` next to your Xray binary.
- **Domain strategy** defaults to **AsIs** (also supports IPIfNonMatch and IPOnDemand).
- Built-in profiles cannot be deleted; duplicate them to customize.
- **Real delay** tests always use **Proxy All** routing so bypass rules do not skew latency measurements.

Profiles are stored in `routing.json` under the app data directory. The active profile is remembered in settings (default: **Bypass LAN** on first run).

## Geo data manager

Xray routing can use `geoip:…` and `geosite:…` tags (for example `geoip:ru`, `geosite:ru`, `geosite:category-ads-all`) when matching `geoip.dat` and `geosite.dat` files are available next to the configured Xray executable.

Open **Tools → Geo Data Manager…** to:

- Check whether `geoip.dat` / `geosite.dat` exist beside your Xray binary
- Download updates from a built-in source (**Loyalsoldier v2ray-rules-dat**, compatible with Xray/V2Ray-style routing)
- Verify downloads using published `.sha256sum` files before replacing existing data
- Open the Xray resource directory in your file manager

**File placement:** Zarya places active geo files in the same directory as the Xray executable (for example `cores/xray/geoip.dat`). If that directory is not writable, use **Settings** to choose another Xray path.

**On start:** If the active routing profile uses geo rules but required files are missing, Zarya can warn before validation (**Open Geo Data Manager**, **Continue**, or **Cancel Start**). Final authority remains Xray `run -test`.

Options in the dialog:

- **Check geo data status on startup** — log presence/missing in the main log (no automatic download)
- **Warn if routing uses geo rules and files are missing**

Releases do not bundle third-party `.dat` files; use **Update All** after first install.

## Experimental TUN mode (0.15+)

Zarya’s default mode remains **system proxy via Xray**. An opt-in **experimental TUN mode** uses **sing-box** as the TUN backend (see `docs/tun-design.md` and `docs/privileged-helper-design.md`).

- **Settings → Experimental** — TUN runtime, routing/DNS profiles, DNS hijack, and **TUN privilege mode** (direct GUI vs `zarya-helper`).
- **`zarya-helper`** — separate executable for privileged TUN start/stop over local IPC (token auth, path restrictions). Not installed as an OS service in 0.15.
- **Tools → Preview sing-box TUN config…** — generated JSON, warnings, Copy/Save, `sing-box check`.
- TUN config uses the same **RoutingProfile** and **DnsProfile** as Xray mode where possible.
- TUN mode does **not** enable OS system proxy.

## sing-box rule sets (0.17)

- **Tools → sing-box Rule Sets** — manage local `.srs` files separately from Xray `geoip.dat`/`geosite.dat`.
- Import `.srs`, compile source JSON via `sing-box rule-set compile`, optional strict mode before TUN start.
- See `docs/sing-box-rule-sets.md`.

## Diagnostics bundle (0.21)

- **Help → Create Diagnostics Bundle…** — redacted `.zarya-diagnostics.zip` for support and debugging.
- Not a backup; secrets, tokens, and raw runtime configs are excluded by design.
- See `docs/diagnostics-bundle.md`.

## Backup import/export (0.20)

- **File → Export Backup…** / **File → Import Backup…** — portable `.zarya-backup.zip` archives.
- Selective export/import, redacted diagnostic mode, checksum verification, automatic pre-import backup.
- See `docs/backup-import-export.md`.

## Core update manager (0.19)

- **Tools → Core Manager** — download, verify, and install **Xray** and **sing-box** from GitHub releases.
- Checksum verification when upstream provides checksum assets; backup and rollback supported.
- Does not update Zarya itself, helper, or Wintun — see `docs/core-update-manager.md`.

## Experimental kill switch (0.16+)

- Implemented in **`zarya-helper`** (requires helper mode, not direct GUI sing-box).
- **Linux:** nftables PoC (`table inet zarya` only; never flushes global ruleset).
- **Windows:** WFP PoC (0.18); **macOS:** unsupported — see `docs/kill-switch-design.md`.
- Recovery: `docs/recovery.md` and Settings → Kill Switch → Show Recovery Instructions.
- For elevated TUN on Windows/macOS/Linux, run `zarya-helper` elevated manually or use dev mode and accept platform limits.

## DNS profiles

Configure Xray's built-in DNS module in **Tools → DNS Profiles…** or **Settings → DNS**.

Built-in profiles:

- **System DNS** — Zarya omits the Xray `dns` object (default Xray/system behavior).
- **Secure Remote DNS** — Cloudflare and Google DoH with `queryStrategy: UseIP`.
- **China Direct / Global Remote** — template using `geosite:cn` / `geoip:cn` and remote DoH for other regions.
- **Custom** — edit servers, hosts, and advanced flags.

The active DNS profile is stored in `dns.json` and included in generated configs when you **Start** a profile.

**Important limitations:**

- DNS profiles control how **Xray resolves domains** for its routing logic. They do not hijack all OS DNS queries.
- **System proxy mode** does not capture every application's DNS traffic. Full DNS capture requires TUN/local DNS inbound (planned separately).
- Profiles using `geosite:` / `geoip:` need `geosite.dat` / `geoip.dat` next to Xray (see Geo Data Manager).
- Zarya may warn when routing uses geo rules with **System DNS**, or when DNS/routing combinations look risky — you can continue; `xray run -test` remains the final check.

## Cross-platform system proxy support

Zarya can enable a **local HTTP proxy** as the OS/desktop system proxy. This is not TUN/VPN mode — only applications that respect system proxy settings are affected.

| Platform | Backend | Support |
|----------|---------|---------|
| Windows | WinINet registry | Full |
| macOS | `networksetup` | Full (may require admin on some systems) |
| Linux GNOME | `gsettings` / `org.gnome.system.proxy` | Full when `gsettings` is available |
| Linux KDE/Plasma | — | Partial / unsupported in this milestone |
| Other Linux | — | Unsupported |

- **macOS**: applies HTTP and HTTPS proxy to a selected network service (or all services if configured in Settings). Previous per-service proxy state is restored on stop/exit.
- **Linux GNOME**: sets manual HTTP/HTTPS proxy to the local inbound port. Previous gsettings values are restored on stop/exit.
- **Linux KDE**: reported as partial; Zarya does not modify KDE proxy settings yet. Core can still run; auto system proxy logs a clear message.
- **CLI tools** on Linux may need `http_proxy` / `https_proxy` environment variables manually.

TUN transparent proxy is planned separately.

## Startup behavior

Settings → **Startup**:

- **Start Zarya when I log in** — OS autostart (Windows Run key, macOS LaunchAgent, Linux XDG autostart)
- **Start minimized to tray** — hide the main window on launch
- **Auto-start last used profile** — after a delay, start the last profile you ran manually
- **Enable system proxy after auto-starting profile** — separate from manual-start proxy option

On launch, Zarya can wait a few seconds after login before auto-starting a profile so the desktop session is ready.

## Portable mode

Place `portable.flag` next to the executable, or pass `--portable`.

Data is stored under `./data` (profiles, subscriptions, routing, `settings.ini`). Runtime files go under `./runtime`. Core binaries are expected under `./cores/xray/`.

Non-portable mode continues to use the OS app data directory.

## CLI arguments

| Argument | Description |
|----------|-------------|
| `--portable` / `-p` | Use app-local `data/` directory |
| `--minimized` / `-m` | Start hidden to tray |
| `--no-autostart-profile` | Skip auto-start of last profile |
| `--start-profile <id>` | Start a specific profile after launch |
| `--log-level <level>` | `debug`, `info`, `warn`, or `error` |

## Packaging

| Platform | Artifact | Script |
|----------|----------|--------|
| Windows | `Zarya-0.27.0-beta-windows-x64-portable.zip` | `scripts/package-windows.ps1` |
| macOS | `Zarya-0.27.0-beta-macos-<arch>.zip` | `scripts/package-macos.sh` |
| Linux | `Zarya-0.27.0-beta-linux-<arch>.tar.gz` | `scripts/package-linux.sh` |

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist -SkipSigning
python scripts\run-smoke-tests.py --artifact .\dist\Zarya-0.27.0-beta-windows-x64-portable.zip --build-dir build
python scripts\verify-release-artifacts.py --artifact .\dist\Zarya-0.27.0-beta-windows-x64-portable.zip --expected-version 0.27.0-beta --require-checksum --allow-unsigned
```

See [docs/release-packaging.md](docs/release-packaging.md), [docs/signing/README.md](docs/signing/README.md), and `packaging/windows/portable-layout.md`. Artifacts include `release-manifest.json`, SHA256 checksums, translations, docs, and core placeholders. Xray/sing-box are not bundled. Signing is optional in 0.27; in-app auto-update is not included.

## Verifying downloads

Each release artifact includes a SHA256 checksum.

### Windows/macOS/Linux

Use `SHA256SUMS.txt` or the per-artifact `.sha256` file.

```bash
sha256sum -c SHA256SUMS.txt
```

Signed builds are not required for 0.27 beta. Optional signing hooks are documented in [docs/signing/](docs/signing/).

## Supported runnable protocols (Xray)

| Protocol | Transports | Security | Notes |
|----------|------------|----------|--------|
| VLESS | tcp | reality, tls, none | REALITY requires tcp + public key + SNI |
| VMess | tcp, ws, grpc | none, tls | VMess may fail if system UTC time is wrong |
| Trojan | tcp, ws | tls, none, reality (tcp) | Password required |
| Shadowsocks | tcp | — | Method preserved as imported; **no plugin** |
| SOCKS | — | — | Optional outbound |

**Imported but not runnable yet:** Shadowsocks with `plugin=`, exotic transports (xhttp), Clash YAML providers.

## Usage (0.12)

1. Launch **zarya**.
2. Configure **Xray** path in Settings if needed.
3. **Add** a VLESS REALITY profile manually, or **Import VLESS link…**.
4. **Save** writes `profiles.json`; **Load** reloads from disk.
5. Select a profile and click **Start**:
   - Config is generated and written to the runtime path.
   - `xray run -test -config …` runs; on failure, a dialog and log output appear.
   - On success, `xray run -config …` starts.
   - If **auto system proxy** is enabled (Windows), WinINet proxy is set to the local HTTP inbound.
6. **Stop** restores system proxy (if Zarya changed it), then terminates Xray (terminate, wait 3s, then kill).
7. Core stdout/stderr appear in the log panel.
8. Status bar shows **Core**, **System proxy**, and **Routing** profile name.
9. Set routing in **Tools → Routing Profiles…** or **Settings → Routing**.
10. For bypass profiles, open **Tools → Geo Data Manager…** and run **Update All** if `geoip.dat` / `geosite.dat` are missing.
11. Choose a **DNS profile** in Settings or **Tools → DNS Profiles…** (default: **System DNS**).

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
  core/       XrayAdapter, stream settings, CoreManager
  import/     VlessUriParser
  storage/    ProfileStore, SubscriptionStore, RoutingStore, AppSettings, AppPaths
  routing/    RoutingManager, XrayRoutingGenerator, RoutingGeoUtils
  geodata/    GeoDataManager, downloader, verifier
  dns/        DnsManager, XrayDnsGenerator, DnsValidator
  testing/    TcpPingTester, RealDelayTester, TestManager
  platform/   Default core executable paths
```

## Current limitations

- **Xray**: VLESS, VMess, Trojan, Shadowsocks (see table above); sing-box still stub.
- **sing-box**: adapter stub only; cannot start.
- **System proxy**: Windows (full), macOS `networksetup` (full), Linux GNOME `gsettings` (full); KDE partial; no TUN/PAC.
- **Subscriptions**: no scheduled auto-update; no Clash/sing-box subscription formats.
- No DNS editor, adblock rule providers, TUN mode, speedtest/download benchmark, or auto best-node selection.
- Beta packaging scripts exist; signed installers and store publishing are not included.
- Milestone 0.1 `profiles.json` files still load; missing fields get safe defaults.

Expected log after **Start** (with auto system proxy on Windows):

```
Generating config…
Config path: …
Validating Xray config…
Validation OK
Starting Xray…
Xray started
SOCKS: 127.0.0.1:10808
HTTP: 127.0.0.1:10809
Reading current Windows proxy settings…
Previous proxy state saved
Applying system proxy: http=127.0.0.1:10809;https=127.0.0.1:10809
Windows proxy settings changed notification sent.
```

Run `zarya_xray_config_test` (or `.\scripts\run-xray-config-test.ps1`) to verify REALITY JSON generation.

## License

See [LICENSE](LICENSE).
