# Zarya TUN Mode Design

## Goal

Add optional transparent proxy (TUN) mode to Zarya without breaking the existing system-proxy workflow. Milestone 0.13 is a **design spike** and **minimal sing-box proof of concept**, not production TUN.

## Non-goals (0.13)

- Production-ready kill switch
- macOS Network Extension packaging
- Windows service installation or driver installers
- Full routing/DNS parity with Xray routing and DNS profiles
- Per-app split tunneling
- Automatic privilege escalation (sudo / UAC)
- Signed driver redistribution
- Enabling TUN by default in releases

## Current modes

| Mode | Backend | OS proxy | Status |
|------|---------|----------|--------|
| System proxy via Xray | `xray run` + local SOCKS/HTTP inbounds | WinINet / networksetup / gsettings | Default, production path |
| TUN via sing-box (experimental) | `sing-box run` + `tun` inbound | Not used | Opt-in, PoC |

## Why sing-box for the first TUN backend

- sing-box documents a first-class **TUN inbound** with `auto_route`, sniffing, and DNS integration.
- Xray has TUN-related pieces, but client ecosystems (Clash/sing-box) treat sing-box as the more practical transparent-proxy stack for desktop clients.
- Zarya already ships a sing-box adapter stub; 0.13 extends it for TUN PoC while keeping **Xray as the primary system-proxy core**.

## Platform model

### Windows

- Route changes and TUN creation typically require **administrator** privileges.
- Many builds depend on **Wintun** (userspace L3 TUN). Zarya does not install Wintun in 0.13.
- Risks: stale routes after crash, conflicting VPN clients, corporate policy blocking TUN.

### macOS

- CLI TUN may work in some setups with elevated privileges.
- **Production-grade** macOS VPN-like behavior usually requires **Network Extension** (Packet Tunnel Provider), app signing, entitlements, and notarization.
- 0.13 documents this limitation; Network Extension is out of scope.

### Linux

- Requires `/dev/net/tun` and usually **root** or `CAP_NET_ADMIN`.
- `ip` from iproute2 is expected for route management (sing-box `auto_route`).
- Future kill switch may use **nftables/iptables** (not in 0.13).

## DNS behavior (PoC)

- PoC sing-box config uses a minimal internal DNS block: remote DoH via proxy detour, `local` for `geosite:private`, `final: remote`.
- Zarya **DNS profiles** (0.12) are **not** mapped into sing-box TUN configs yet.
- No local DNS inbound on `127.0.0.1:53` in 0.13.

## Routing behavior (0.14)

- Active **RoutingProfile** is translated to sing-box `route` via `SingBoxRouteGenerator`.
- Built-ins: Proxy All (`final: proxy`), Bypass LAN (`ip_is_private` + `geosite:private`), Bypass RU (`geosite`/`geoip`), Custom rules (domain/IP/port/protocol mapping).
- Rule order: Block → Direct → Proxy → `final: proxy` (aligned with Xray generator ordering).
- `geoip:private` maps to `ip_is_private: true` (does not depend on geo rule data).
- Port ranges and some protocol rules may warn and be skipped; **sing-box check** is final authority.

## DNS behavior (0.14)

- Active **DnsProfile** is translated to sing-box `dns` via `SingBoxDnsGenerator`.
- System DNS in TUN: `local` server + leak warning.
- Secure Remote: DoH via `detour: proxy`.
- China Direct / Global Remote: split DNS with geosite rules (warnings if rule-sets missing).
- Optional **DNS hijack** route rule (`hijack-dns`) when enabled in Settings.

## Geo data compatibility

- Xray uses `geoip.dat` / `geosite.dat` next to the Xray executable (Geo Data Manager).
- sing-box may require its own **rule-set** format (`.srs`) under `data/sing-box/rule-set/`.
- 0.14 translates `geosite:` / `geoip:` matchers and warns when rule-sets are missing; no sing-box rule-set downloader yet.

## Validation flow

1. Generate sing-box TUN config from profile + active routing + active DNS.
2. Show warnings (blocking issues prevent start).
3. Write `runtime/sing-box-tun.json`.
4. Run `sing-box check -c …` before `sing-box run`.
5. On failure: show stderr/stdout; offer preview.

## Safe shutdown

- Normal exit: stop sing-box process, wait, kill if needed; do not touch system proxy in TUN mode.
- Route cleanup is delegated to sing-box process teardown in 0.13 (no separate route restore layer).

## Crash recovery

- Settings track `lastShutdownClean`, `tunWasRunning`, `lastRuntimeMode`.
- On next launch, if previous TUN session ended uncleanly, show a warning: user may need to kill sing-box or reboot; automatic route recovery is **not** implemented.

## Kill switch plan (future)

| Platform | Direction |
|----------|-----------|
| Windows | WFP / Windows Firewall rules with guaranteed restore |
| macOS | pf rules or Network Extension policies |
| Linux | nftables/iptables |

**0.16:** experimental kill switch in `zarya-helper` (Linux nft PoC). **0.13–0.15:** no kill switch. A bad kill switch is worse than none on unsupported platforms.

## Open questions

1. Ship sing-box in portable bundles or require user-provided binary only?
2. When to invest in macOS Network Extension vs. staying CLI-only on macOS?
3. How much routing/DNS parity is required before calling TUN beta?
4. Privilege helper: setuid helper, Windows service, or polkit (Linux)?
5. Shared config model: one internal IR, two exporters (Xray JSON vs sing-box JSON)?

## Milestone plan

| Milestone | Scope |
|-----------|--------|
| **0.13** | Design doc, runtime backend abstraction, sing-box TUN PoC, experimental settings |
| **0.14** | sing-box routing/DNS parity from RoutingProfile + DnsProfile, config preview, warnings |
| **0.15** | Privileged `zarya-helper` PoC (local IPC, token auth, TUN start/stop); not a production service |
| **0.16** | Experimental kill switch in helper (Linux nftables PoC; Windows WFP / macOS NE documented as future) |
| 0.17+ | sing-box rule-set manager / downloader improvements |
| 0.17+ | Production Windows WFP kill switch, macOS Network Extension |
