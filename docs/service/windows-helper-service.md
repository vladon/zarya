# Windows Helper Service

## Service name

`ZaryaHelper`

## Account

`LocalSystem` for PoC.

## Install

```powershell
sc.exe create ZaryaHelper binPath= "\"C:\Path\zarya-helper.exe\" --service --service-name ZaryaHelper --allowed-runtime-dir \"...\"" start= demand DisplayName= "Zarya Helper"
```

Or use **Settings → Experimental → Helper → Install** (Administrator required).

## Start/Stop

```powershell
sc.exe start ZaryaHelper
sc.exe stop ZaryaHelper
```

## Uninstall

```powershell
sc.exe stop ZaryaHelper
sc.exe delete ZaryaHelper
```

## Recovery

See [helper-recovery.md](helper-recovery.md).

## Limitations

- beta PoC
- not signed service installer
- no automatic helper update
