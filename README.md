# Zarya

Zarya is a cross-platform Qt 6 desktop client for managing proxy profiles and launching external proxy cores (Xray, sing-box). Milestones 0.1–0.28: profiles, subscriptions, Xray, routing, geo data, DNS, system proxy, experimental TUN, sing-box rule sets, core update manager, backup import/export, diagnostics bundle, beta hardening, privileged helper, experimental kill switch (Linux nft / Windows WFP PoC), tray, autostart, English/Russian UI, release packaging, signing-ready hooks, and **0.28.0-beta** helper service design. **0.29.0-beta** adds public beta docs and issue templates. **0.30.0-beta** adds feedback triage, richer diagnostics, and Copy Support Summary. **0.31.0-beta** adds production installer planning and portable-to-installed migration skeleton. **0.32.0-beta** adds app self-update design (manifest check, download-and-verify; no auto-install). **0.33.0-beta** adds stable release hardening — feature gating, 1.0 scope, and release criteria. **0.34.0-beta** adds a Windows MSI installer PoC (WiX); portable ZIP remains recommended. **0.35.0-beta** adds portable app updater PoC via external `zarya-updater` (Windows/Linux portable; installed MSI remains manual). **0.36.0-rc1** was the first release candidate — experimental features disabled by default, RC docs/checklists, redaction audit script, and gated app updater install. **1.0.0** is the first stable release — stable channel defaults, stable release docs/checklists, and the same experimental feature gating without RC/beta pre-release banner wording.

Zarya supports **English** and **Russian** UI. Change language in **Settings → General → Language** (restart required for full effect). See [docs/localization.md](docs/localization.md).

## Requirements

- **CMake** 3.21 or newer
- **C++20** compiler (MSVC 2019+, GCC 10+, Clang 12+)
- **Qt 6.2+** with modules: Core, Gui, Widgets, Network

No other third-party libraries are required for the application itself. **Xray** is optional and only needed when starting a profile.

## Build

### Windows (primary)

Requires **Visual Studio 2026** (or 2022+) with the **Desktop development with C++** workload, plus **Qt 6** built for MSVC (`msvc2022_64` kit — compatible with the VS 2026 toolset).

Local builds use **static Qt only** (portable exe, no Qt DLLs):

```powershell
# One-time: build static Qt (if missing) — ~30–90 min
.\scripts\build-qt-static-msvc2026.ps1

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

```powershell
.\scripts\build-qt-static-msvc2026.ps1    # installs to C:\Qt\Static\6.8.3\msvc2022_64
.\scripts\configure-msvc2026.ps1 -Force
cmake --build build --config Release --target zarya
.\build\Release\zarya.exe                 # portable; Qt compiled in (/MT)
```

Or use preset `windows-msvc2026-static-release` with `QT_STATIC_DIR=C:/Qt/Static/6.8.3/msvc2022_64`.

### macOS

Requires Homebrew Qt 6 (`brew install cmake qt@6`). Wrapper configures if needed (same role as `build.ps1` on Windows):

```bash
./scripts/build-macos.sh
open ./build/zarya.app
```

Options: `--test`, `--force` (reconfigure), `--config Debug`, `--target <name>`, `-j <N>`. Override Qt with `CMAKE_PREFIX_PATH` or `QT_ROOT`.

Manual configure/build:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build
open ./build/zarya.app
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

## Stable status (1.4.0)

Zarya **1.4.0** is the current stable release. The recommended path is **Xray system-proxy mode** (Routing: Bypass LAN, DNS: Secure Remote DNS or System DNS).

Experimental sing-box TUN, zarya-helper, and kill switch exist but are **disabled/hidden by default** in stable builds. See [docs/stable/stable-scope.md](docs/stable/stable-scope.md) and [docs/public-beta/experimental-features.md](docs/public-beta/experimental-features.md).

Quick start: [docs/public-beta/quick-start.md](docs/public-beta/quick-start.md).

## Installation status

Zarya **1.4.0** is distributed primarily as **portable/bundle artifacts** (ZIP, tarball, `.app` archive).

Production installers are planned but not part of the current stable portable distribution:

- Windows — WiX MSI **PoC** ([docs/installer/windows-msi-poc.md](docs/installer/windows-msi-poc.md)); strategy ([docs/installer/windows-installer-strategy.md](docs/installer/windows-installer-strategy.md))
- macOS — DMG/PKG path ([docs/installer/macos-installer-strategy.md](docs/installer/macos-installer-strategy.md))
- Linux — AppImage/deb/rpm path ([docs/installer/linux-packaging-strategy.md](docs/installer/linux-packaging-strategy.md))

Portable mode remains supported. **File → Import from Portable Zarya Folder…** helps migrate data explicitly when moving to an installed layout later.

## App self-update (portable PoC)

Zarya can **check** update manifests and **download/verify** artifacts. **Install is disabled by default** on stable builds.

- **Help → Check for App Updates…** — local manifest or configured URL
- **Settings → App updates** — channel, manifest URL (separate from Core updates)
- Portable install via external `zarya-updater` when explicitly enabled (dev/beta default)
- Docs: [docs/updater/README.md](docs/updater/README.md), [docs/updater/portable-update-implementation.md](docs/updater/portable-update-implementation.md)

Core Manager updates Xray/sing-box. App updates update Zarya itself.

## Stable scope

**1.4.0 stable:** Xray system-proxy desktop client with WireGuard share-link support, silent startup recovery, bundled Xray and runetfreedom geo seeds, and a `lib_ui`-based status surface.

**Experimental (beta/dev or explicit opt-in):** TUN, helper, kill switch.

Docs: [docs/stable/README.md](docs/stable/README.md), [docs/release-notes/1.4.0.md](docs/release-notes/1.4.0.md)

## Windows MSI PoC

WiX-based installer proof of concept. **Portable ZIP remains the recommended 1.4.0 distribution.**

```powershell
.\scripts\package-windows-msi.ps1 -Configuration Release -OutputDir .\dist -SkipSigning
```

See [docs/installer/windows-msi-poc.md](docs/installer/windows-msi-poc.md).

## Quick start

1. Use the bundled Xray under `cores/xray/` (release builds), or open **Tools → Core Manager** to install/update it.
2. Import a profile link (**File → Import Profile Links…**) or add a subscription.
3. Choose **Routing: Bypass LAN** (default) in the setup wizard or **Tools → Routing Profiles**.
4. Choose **DNS: System DNS** (default).
5. Click **Start**.

## First-run setup

Zarya shows a setup wizard on first launch. You can reopen it from **Help → Run Setup Wizard**.

## Configure Xray path

1. Open **File → Settings…**
2. Set **Xray executable** (Browse or paste path).
3. Set **Local proxy port** (default `10808`) — mixed SOCKS5 + HTTP on one port.
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
- Share links: `vless://`, `vmess://`, `trojan://`, `ss://`, `hysteria2://`/`hy2://`, `wireguard://`/`wg://`

**Unsupported:** Clash YAML, sing-box JSON subscriptions, provider metadata blocks.

**Update behavior:**

- **Update Selected** (profile filter set to a subscription) or **Update All**
- Manual profiles are never changed or deleted
- Nodes removed from the remote list are marked `[missing]` (soft-delete), not erased
- Failed updates keep existing profiles unchanged

**Runnable through Xray** (when fields are complete): VLESS (REALITY/TLS/none), VMess (TCP/WS + TLS), Trojan (TCP/WS + TLS/Reality), Shadowsocks (no plugin), Hysteria2 (TLS), WireGuard. Profiles with unsupported features (e.g. SS plugin) stay in the table with an import note.

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

## System proxy controls

Zarya can set the operating-system or desktop HTTP/HTTPS proxy to the local Xray mixed
inbound (`127.0.0.1:<mixedPort>`, default **10808**).

- **Settings → Proxy Mode**: enable proxy automatically when a profile starts and restore
  previous settings on stop/exit.
- **Tools → Enable System Proxy** / **Restore Previous Proxy** for manual control (enable requires a running core).
- Before enabling, Zarya saves every platform proxy value it changes and restores the exact
  snapshot on **Stop**, safe exit, or startup recovery.
- The confirmation dialog identifies the change as an OS/desktop proxy operation rather than a
  Windows-only operation.

**Notes:**

- Only applications that respect the selected OS/desktop proxy backend are affected.
- CLI tools often need explicit `HTTP_PROXY` / `HTTPS_PROXY` and are out of scope.

Windows state can be verified in PowerShell:

```powershell
Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings' |
  Select-Object ProxyEnable, ProxyServer, ProxyOverride, AutoDetect, AutoConfigURL
```

When Zarya enables proxy, expect roughly:

```
ProxyEnable = 1
ProxyServer = http=127.0.0.1:10808;https=127.0.0.1:10808
ProxyOverride = <local>
```

## Desktop behavior

- Closing the main window can **hide Zarya to the system tray** instead of quitting (default on Windows when a tray is available).
- Use **tray → Exit** or **File → Exit** to fully quit.
- On full exit, Zarya stops the core, cancels tests, and attempts to restore the previous system
  proxy settings.
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
- **Real delay** starts a separate temporary Xray process (not the running core) and measures HTTP time to the test URL (default [`https://www.cloudflare.com/cdn-cgi/trace`](https://www.cloudflare.com/cdn-cgi/trace)).
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
- Download updates from a built-in source (compatible with Xray/V2Ray-style routing):
  - **runetfreedom russia-v2ray-rules-dat** (default; Russia-focused; also the release seed)
  - **Loyalsoldier v2ray-rules-dat** (general-purpose)
  - **Chocolate4U Iran-v2ray-rules** (Iran-focused)
- Verify downloads using published `.sha256sum` files before replacing existing data
- Open the Xray resource directory in your file manager

**File placement:** Zarya places active geo files in the same directory as the Xray executable (for example `cores/xray/geoip.dat`). If that directory is not writable, use **Settings** to choose another Xray path.

**On start:** If the active routing profile uses geo rules but required files are missing, Zarya can warn before validation (**Open Geo Data Manager**, **Continue**, or **Cancel Start**). Final authority remains Xray `run -test`.

Options in the dialog:

- **Check geo data status on startup** — log presence/missing in the main log (no automatic download)
- **Warn if routing uses geo rules and files are missing**

Release packages ship a pinned runetfreedom seed next to Xray; use **Update All** later to refresh or switch sources.

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
| Linux KDE/Plasma | `kioslaverc` via KConfig 6 or 5 | Full when KConfig tools and the session D-Bus are available |
| Other Linux | — | Unsupported |

- **macOS**: applies HTTP and HTTPS proxy to a selected network service (or all services if configured in Settings). Previous per-service proxy state is restored on stop/exit.
- **Linux GNOME**: sets manual HTTP/HTTPS proxy to the local inbound port. Previous gsettings values are restored on stop/exit.
- **Linux KDE/Plasma**: sets manual HTTP/HTTPS proxy in `kioslaverc`, notifies running KIO
  applications, and restores the exact prior mode, endpoints, bypass rules, and PAC setting.
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
| Windows (portable) | `Zarya-1.4.0-windows-x64-portable.zip` | `scripts/package-windows.ps1` |
| Windows (MSI PoC) | `Zarya-1.4.0-windows-x64-installer-poc.msi` | `scripts/package-windows-msi.ps1` |
| macOS | `Zarya-1.4.0-macos-<arch>.zip` | `scripts/package-macos.sh` |
| Linux | `Zarya-1.4.0-linux-<arch>.tar.gz` | `scripts/package-linux.sh` |

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist -SkipSigning
python scripts\run-smoke-tests.py --artifact .\dist\Zarya-1.4.0-windows-x64-portable.zip --build-dir build
python scripts\verify-release-artifacts.py --artifact .\dist\Zarya-1.4.0-windows-x64-portable.zip --expected-version 1.4.0 --release-stable --allow-unsigned
```

See [docs/release-packaging.md](docs/release-packaging.md), [docs/release/release-process.md](docs/release/release-process.md), [docs/signing/README.md](docs/signing/README.md), and `packaging/windows/portable-layout.md`. Artifacts include `release-manifest.json`, SHA256 checksums, translations, docs, a **bundled Xray seed** under `cores/xray/`, and **pinned runetfreedom geo data** (`geoip.dat` / `geosite.dat`) in the same folder (sing-box is not bundled). Signing is optional.

## Verifying downloads

Each release artifact includes a SHA256 checksum. See [docs/public-beta/download-verification.md](docs/public-beta/download-verification.md) and [docs/release/release-process.md](docs/release/release-process.md).

### Windows/macOS/Linux

Use `SHA256SUMS.txt` or the per-artifact `.sha256` file.

```bash
sha256sum -c SHA256SUMS.txt
```

Signed builds are optional. See [docs/signing/](docs/signing/).

## Diagnostics

**Help → Create Diagnostics Bundle** creates a redacted archive for troubleshooting. **Help → Copy Support Summary** copies a short redacted summary. Review before sharing. See [docs/public-beta/privacy-and-diagnostics.md](docs/public-beta/privacy-and-diagnostics.md).

## Reporting issues

1. Create a diagnostics bundle (**Help → Create Diagnostics Bundle**) or copy a support summary (**Help → Copy Support Summary**).
2. Follow [docs/public-beta/reporting-issues.md](docs/public-beta/reporting-issues.md).
3. Do not post raw proxy links, subscription URLs, passwords, or helper tokens in public issues.

## Supported runnable protocols (Xray)

| Protocol | Transports | Security | Notes |
|----------|------------|----------|--------|
| VLESS | tcp | reality, tls, none | REALITY requires tcp + public key + SNI |
| VMess | tcp, ws, grpc | none, tls | VMess may fail if system UTC time is wrong |
| Trojan | tcp, ws | tls, none, reality (tcp) | Password required |
| Shadowsocks | tcp | — | Method preserved as imported; **no plugin** |
| Hysteria2 | hysteria (QUIC) | tls | Auth/password; ALPN defaults to `h3` |
| WireGuard | udp | — | Private key + peer public key; optional local address / MTU / reserved / PSK |
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
   - If **auto system proxy** is enabled (Windows), WinINet proxy is set to the local mixed inbound.
6. **Stop** restores system proxy (if Zarya changed it), then terminates Xray (terminate, wait 3s, then kill).
7. Core stdout/stderr appear in the log panel.
8. Status bar shows **Core**, **System proxy**, and **Routing** profile name.
9. Set routing in **Tools → Routing Profiles…** or **Settings → Routing**.
10. For bypass profiles, open **Tools → Geo Data Manager…** and run **Update All** if `geoip.dat` / `geosite.dat` are missing.
11. Choose a **DNS profile** in Settings or **Tools → DNS Profiles…** (default: **System DNS**).

Local mixed inbound when Xray starts (from generated config, port from Settings):

- Mixed (SOCKS5 + HTTP): `127.0.0.1:10808` (default)

Expected log sequence after **Start**:

```
Generating config…
Config path: …
Validating Xray config…
Validation OK
Starting Xray…
Xray started
Proxy: 127.0.0.1:10808 (mixed SOCKS/HTTP)
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

- **Xray**: VLESS, VMess, Trojan, Shadowsocks, Hysteria2, WireGuard (see table above); sing-box still stub.
- **sing-box**: adapter stub only; cannot start.
- **System proxy**: Windows WinINet, macOS `networksetup`, Linux GNOME `gsettings`, and Linux
  KDE/Plasma KConfig are supported when their native facilities are available. This is not TUN;
  PAC configurations are preserved and restored but Zarya itself applies a manual local proxy.
- **Subscriptions**: no scheduled auto-update; no Clash/sing-box subscription formats.
- No DNS editor, adblock rule providers, TUN mode, speedtest/download benchmark, or auto best-node selection.
- Beta packaging scripts exist; signed installers and store publishing are not included.
- Milestone 0.1 `profiles.json` files still load; missing fields get safe defaults.

Expected backend-neutral log after **Start** with automatic system proxy:

```
Generating config…
Config path: …
Validating Xray config…
Validation OK
Starting Xray…
Xray started
Proxy: 127.0.0.1:10808 (mixed SOCKS/HTTP)
System proxy backend: …
Reading current proxy state…
Previous proxy state saved
Applying HTTP/HTTPS proxy 127.0.0.1:10808
Applying proxy settings via …
System proxy applied successfully.
```

Run `zarya_xray_config_test` (or `.\scripts\run-xray-config-test.ps1`) to verify REALITY JSON generation.

## License

Zarya is **dual-licensed** under the [MIT License](LICENSE.MIT) and
[GPLv3+](LICENSE.GPL-3.0). See [LICENSE](LICENSE) for how each applies.

Official builds always link Desktop App Toolkit UI (`lib_ui`) and are
distributed under **GPLv3+** ([desktop-app/lib_ui](https://github.com/desktop-app/lib_ui),
[LEGAL](https://github.com/desktop-app/legal/blob/master/LEGAL)). Zarya-authored
source may still be used under MIT or GPLv3+ per [LICENSE](LICENSE).
