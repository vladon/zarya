# Linux systemd + polkit Helper

## systemd unit

Template: `packaging/linux/systemd/zarya-helper.service`

## polkit actions

Template: `packaging/linux/polkit/dev.vladon.zarya.helper.policy`

Actions:

- `dev.vladon.zarya.helper.manage-service`
- `dev.vladon.zarya.helper.start-tun`
- `dev.vladon.zarya.helper.stop-tun`
- `dev.vladon.zarya.helper.manage-killswitch`

## Manual install

```bash
sudo install -m 0755 zarya-helper /usr/lib/zarya/zarya-helper
sudo install -m 0644 packaging/linux/systemd/zarya-helper.service /etc/systemd/system/zarya-helper.service
sudo systemctl daemon-reload
sudo systemctl enable --now zarya-helper.service
```

## Manual uninstall

```bash
sudo systemctl stop zarya-helper.service
sudo systemctl disable zarya-helper.service
sudo rm /etc/systemd/system/zarya-helper.service
sudo systemctl daemon-reload
```

## Limitations

0.28 does not automatically install the system service from the GUI.
