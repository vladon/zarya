# Windows WFP Kill Switch PoC

## Goal

Provide an experimental Windows kill switch for TUN mode using the **Windows Filtering Platform (WFP)** inside `zarya-helper`. WFP is the standard Windows network filtering platform; applications manage filters through the WFP engine (`FwpmEngineOpen0`, `FwpmFilterAdd0`, etc.).

The PoC blocks outbound connection attempts except:

- loopback
- the selected proxy server IP:port
- optional traffic via the TUN interface (best-effort)

## Non-goals

- Kernel-mode callout driver
- Boot-time or persistent filters
- Windows service installer
- Production WFP hardening
- Per-app split tunneling
- System-proxy-mode kill switch
- Windows Firewall `netsh` backend

## WFP objects

Zarya owns stable GUIDs for:

| Object | Purpose |
|--------|---------|
| Zarya provider | Groups Zarya kill switch objects |
| Zarya sublayer | Orders Zarya filters by weight |
| Allow loopback (v4/v6) | Permit `127.0.0.0/8` and `::1` |
| Allow proxy (v4/v6) | Permit resolved proxy IP:port |
| Allow TUN interface (v4/v6) | Optional interface LUID allow |
| Block outbound (v4/v6) | Catch-all block at low weight |

## Layers used

- `FWPM_LAYER_ALE_AUTH_CONNECT_V4`
- `FWPM_LAYER_ALE_AUTH_CONNECT_V6`

These layers control outbound connection authorization.

## Rule order

Within the Zarya sublayer, higher filter weight is evaluated first:

1. Allow loopback
2. Allow proxy server IP:port
3. Optional allow TUN interface
4. Block all other outbound connect attempts

All changes run inside a WFP transaction. Failed enable aborts the transaction so partial rules are not left behind.

## Privileges

Managing WFP filters requires an elevated `zarya-helper` (Administrator). The Base Filtering Engine (BFE) service must be running.

## Recovery

Preferred:

```powershell
zarya-helper --recover-killswitch
```

From the GUI:

1. Settings → Kill Switch → **Disable Now**
2. Or **Show Recovery Instructions**

Manual inspection:

```powershell
netsh wfp show state
```

Zarya only deletes its own provider/sublayer filters by stable GUID. Do not remove unrelated WFP state.

## Helper CLI

```powershell
zarya-helper --killswitch-status
zarya-helper --recover-killswitch
```

These commands do not require `--token-file`.

## Marker file

On enable, helper writes `runtime/killswitch-active.json` with backend `windows-wfp`, provider/sublayer keys, and filter keys. The marker is removed on clean disable.

## Known limitations

- PoC only — not production hardened
- Not boot-time; filters are runtime/session scoped (no `FWPM_FILTER_FLAG_PERSISTENT`)
- Requires Administrator helper
- TUN interface detection is best-effort (`zarya-tun`, `wintun`, `sing-box` name hints)
- IPv6 parity is implemented but may vary by layer/condition support
- LAN allow is not implemented on Windows in 0.18 (Linux nft PoC still supports it)
