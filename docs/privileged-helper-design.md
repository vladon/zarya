# Zarya Privileged Helper Design

## Problem

Experimental TUN mode requires route changes and often elevated privileges. Running the entire Zarya GUI as Administrator or root is undesirable: it widens the attack surface and complicates everyday use.

## Goals

- Keep the GUI running as a normal user.
- Delegate privileged sing-box TUN start/stop to a separate `zarya-helper` process.
- Use local-only IPC with a simple session token.
- Restrict helper to approved config and core paths.
- Provide a development PoC that can evolve into platform services later.

## Non-goals (0.15)

- Production service installer
- Signed Windows service
- macOS SMJobBless / LaunchDaemon installer
- Linux systemd package / polkit integration
- Kill switch or firewall control
- Wintun driver installation
- Remote API or multi-user access

## Architecture

```
Zarya GUI (user)  --QLocalSocket/JSON-->  zarya-helper  --QProcess-->  sing-box TUN
```

The GUI still generates `runtime/sing-box-tun.json` from Profile + RoutingProfile + DnsProfile (0.14). The helper validates paths, runs `sing-box check`, then `sing-box run`.

Xray system-proxy mode remains in the GUI process only.

## IPC transport

- **QLocalServer** / **QLocalSocket** (not TCP localhost).
- Socket name: `zarya-helper-<username>` on Windows, `zarya-helper-<uid>` on Unix.

## Authentication

1. GUI creates a random session token at startup.
2. Token is written to `runtime/helper.token` (user-only where possible).
3. GUI starts helper with `--token-file <path>`.
4. Every IPC request includes the token; mismatch is rejected.

## Command model

| Command | Purpose |
|---------|---------|
| `hello` | Version, platform, privilege, TUN support |
| `status` | sing-box running, pid, config path |
| `checkSupport` | Privilege + TUN support for a sing-box path |
| `validateConfig` | Run sing-box check on config path |
| `startTun` | Validate (optional) and start sing-box |
| `stopTun` | Stop sing-box |
| `shutdownHelper` | Exit helper |

Messages are newline-delimited JSON envelopes (`version`, `id`, `type`, `command`, `token`, `payload`).

## Security boundaries

Helper accepts only:

- `configPath` under `--allowed-runtime-dir`
- `singBoxPath` under `--allowed-core-dir` (when set)
- `workingDirectory` under runtime dir or sing-box parent directory

Arbitrary paths are rejected.

## Platform plans

### Windows

- **0.15:** GUI may start `zarya-helper` in `--dev` mode, or user runs elevated helper manually (`RunAs`).
- **Future:** Windows service with fixed ACL on the named pipe/socket.

### macOS

- **0.15:** Manual `sudo ./zarya-helper --service --token-file …` for privileged tests.
- **Future:** LaunchDaemon or SMJobBless-style helper. Network Extension remains a separate track.

### Linux

- **0.15:** Manual `sudo ./zarya-helper --service --token-file …`.
- **Future:** systemd user/system unit + polkit.

## Helper lifecycle modes

| Mode | Flag | Use |
|------|------|-----|
| Development | `--dev` | GUI starts helper (not auto-elevated) |
| Service | `--service` | User/service starts helper elevated |

Automatic elevation is **not** implemented in 0.15.

## Crash recovery

- GUI safe shutdown sends `stopTun` when helper mode is active.
- If helper is unreachable, user is warned that networking may remain affected.
- No automatic route restoration in 0.15.

## Limitations

- Helper is experimental; not installed as an OS service.
- Token auth is local PoC, not cryptography.
- One primary client connection is assumed for log streaming.
- Geo/rule-set parity is unchanged from 0.14.

## Kill switch (0.16+)

Kill switch firewall rules are owned by the helper (see `docs/kill-switch-design.md`). GUI sends `killSwitchEnable` / `killSwitchDisable` over the same IPC channel.

## Future milestones

- Platform service installers
- Windows WFP kill switch backend (Linux nft PoC in 0.16)
- Firewall and route recovery
- Optional config generation inside helper
