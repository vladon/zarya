# Zarya Kill Switch Design

## Goal

Prevent direct traffic leaks when experimental TUN mode is expected to carry traffic. If sing-box TUN stops or crashes while kill switch is enabled, output traffic should be blocked except loopback, optional LAN, traffic over the TUN interface, and the selected proxy server IP:port.

## Non-goals (0.16)

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

### Windows (0.16 design stub)

- Reports unsupported; documents WFP as production direction
- No firewall rules installed

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
- Windows/macOS do not provide real kill switch in 0.16

## Future milestones

- Windows WFP backend
- macOS Network Extension or controlled PF anchor with explicit opt-in
- systemd / Windows service integration
- Route recovery coordination with TUN teardown
