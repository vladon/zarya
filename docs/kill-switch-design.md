# Zarya Kill Switch Design

## Goal

Prevent direct traffic leaks when experimental TUN mode is expected to carry traffic. If sing-box TUN stops or crashes while kill switch is enabled, output traffic should be blocked except loopback, optional LAN, traffic over the TUN interface, and the selected proxy server IP:port.

## Non-goals

- Production Windows WFP provider or callout driver
- Kernel driver
- macOS Network Extension firewall
- Signed service installer
- Permanent management of third-party firewall products
- Per-app split tunneling
- System-proxy-mode kill switch
- Stealth “block everything” enterprise firewall

## Why helper owns kill switch

Firewall and routing changes require elevated privileges. The GUI runs as a normal user and sends IPC commands to `zarya-helper`, which applies platform-specific rules.

## Platform backends

### Linux nftables PoC (0.16)

- Creates isolated table `inet zarya` only
- Never runs `nft flush ruleset`
- Allows loopback, optional LAN, `zarya-tun`, proxy IP:tcp port
- Optionally blocks direct DNS (UDP/TCP 53)
- Final `reject` on other output traffic
- Rules file: `runtime/killswitch/zarya-nft.conf`
- Recovery: `sudo nft delete table inet zarya`

### Windows WFP PoC (0.18)

- Real WFP backend in `zarya-helper` (`windows-wfp`)
- Zarya-owned provider/sublayer and filters on `ALE_AUTH_CONNECT` v4/v6
- Allows loopback, proxy IP:port, optional TUN interface; blocks other outbound connect
- Transaction-wrapped enable/disable; stable GUIDs for recovery
- Requires Administrator helper and running BFE service
- See [windows-wfp-kill-switch.md](windows-wfp-kill-switch.md)

### macOS (0.16 unsupported)

- PF is not a stable public API for third-party apps (Apple technote)
- No PF rules applied; documents Network Extension as future path

## DNS considerations

Kill switch works best with TUN DNS hijack enabled. If DNS hijack is disabled, the GUI logs a warning that DNS may block or leak depending on OS behavior.

## IPC commands

- `killSwitchStatus`
- `killSwitchCheckSupport`
- `killSwitchEnable`
- `killSwitchDisable`
- `killSwitchRecover`

## Authentication

Same session token model as other helper IPC (0.15).

## Crash recovery

When kill switch is enabled, helper writes `runtime/killswitch-active.json`. On clean disable the marker is removed. On helper/GUI startup, a present marker sets status `needs-recovery` and the GUI offers disable / recovery instructions.

## User recovery procedure

See [recovery.md](recovery.md).

## Risks

- Incorrect rules can block all network access until manual recovery
- Linux PoC requires root/sudo helper
- Windows WFP PoC can block network access if misconfigured; use recovery CLI
- macOS does not provide real kill switch yet

## Future milestones

- macOS Network Extension or controlled PF anchor with explicit opt-in
- Production WFP hardening and optional persistent service
- systemd / Windows service integration
- Route recovery coordination with TUN teardown
