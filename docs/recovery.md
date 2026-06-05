# Zarya Recovery Procedures

## Kill switch (experimental)

### Linux nftables PoC

```bash
sudo nft list tables
sudo nft list table inet zarya
sudo nft delete table inet zarya
```

Zarya only manages `table inet zarya` and does not flush the global ruleset.

### Windows (0.16)

No Zarya kill switch firewall rules are installed by default in this milestone. Future versions may create rules under group **Zarya Kill Switch**. Production should use Windows Filtering Platform (WFP).

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
