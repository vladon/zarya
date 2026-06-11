# Zarya Recovery Procedures

## Unclean shutdown (GUI)

If Zarya was killed while a profile was running, the next startup may show **Startup Recovery**:

- Restore system proxy (when Zarya-owned proxy restore on exit is enabled)
- Recover kill switch (when marker is present)
- Clean runtime temp config files (`xray.json`, `sing-box.json`)

Zarya persists the **pre-enable system proxy snapshot** to `data/proxy-previous-state.json` when it first enables system proxy. After a crash, startup recovery can restore that snapshot or clear a Zarya-owned `127.0.0.1:<http-port>` proxy if no snapshot exists.

Use **Recover** to apply selected actions, **Skip** to continue without changes, or follow kill-switch / TUN sections below if networking is still broken.

Recovery remains available even when experimental features are hidden on the stable release channel.

## Kill switch (experimental)

### Linux nftables PoC

```bash
sudo nft list tables
sudo nft list table inet zarya
sudo nft delete table inet zarya
```

Zarya only manages `table inet zarya` and does not flush the global ruleset.

### Windows WFP PoC (0.18)

Preferred:

```powershell
zarya-helper --recover-killswitch
```

Status:

```powershell
zarya-helper --killswitch-status
```

Manual inspection:

```powershell
netsh wfp show state
```

Zarya only removes its own WFP provider/sublayer filters. See [windows-wfp-kill-switch.md](windows-wfp-kill-switch.md).

### macOS (0.16)

Zarya does not install PF rules in this milestone.

### From the GUI

1. Settings → **Kill Switch (experimental)** → **Disable Now**
2. Or **Show Recovery Instructions**
3. On startup, if a recovery marker is detected: **Disable Kill Switch** or follow instructions

### Marker file

`runtime/killswitch-active.json` — if present after a crash, remove rules manually or use helper **Disable** / **Recover** IPC.

## TUN mode (without kill switch)

If networking is broken after experimental TUN:

1. Stop sing-box / Zarya TUN from the GUI
2. Stop any remaining `sing-box` process
3. Reboot if routes remain wrong (automatic route recovery is not implemented yet)
