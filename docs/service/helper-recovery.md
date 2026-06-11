# Helper Recovery

## Windows

```powershell
sc.exe stop ZaryaHelper
sc.exe delete ZaryaHelper
zarya-helper --recover-killswitch
```

## Linux

```bash
sudo systemctl stop zarya-helper.service
sudo systemctl disable zarya-helper.service
sudo rm /etc/systemd/system/zarya-helper.service
sudo systemctl daemon-reload
zarya-helper --recover-killswitch
```

## macOS (design note)

```bash
sudo launchctl bootout system/dev.vladon.zarya.helper
```

Not active in 0.28 unless a helper is manually installed in the future.

## CLI helpers

- `zarya-helper --service-install-plan`
- `zarya-helper --service-self-test`
- `zarya-helper --recover-killswitch`
- `zarya-helper --stop-runtime`
