# Installed Layout

## Windows target

```
C:\Program Files\Zarya\
  Zarya.exe
  zarya-helper.exe
  translations\
  docs\
  cores\
    xray\
    sing-box\
  .zarya-installed

%AppData%\Zarya\
  profiles.json
  subscriptions.json
  routing.json
  dns.json
  settings.ini
  backups\
  diagnostics\

%LocalAppData%\Zarya\
  runtime\
  logs\
  core-updates\
```

Service (optional): `ZaryaHelper`

## macOS target

```
/Applications/Zarya.app
  Contents/MacOS/Zarya
  Contents/MacOS/zarya-helper
  Contents/Resources/translations
  Contents/Resources/docs

~/Library/Application Support/Zarya/
  profiles.json
  subscriptions.json
  routing.json
  dns.json
  backups

~/Library/Caches/Zarya/
  runtime
  logs
```

Helper future: LaunchDaemon / SMAppService-managed helper.

## Linux target

**AppImage/tarball:**

```
~/Applications/Zarya/
  zarya
  zarya-helper
  translations/
  docs/
  cores/
```

**deb/rpm future:**

```
/usr/bin/zarya
/usr/lib/zarya/zarya-helper
/usr/share/zarya/translations
/usr/share/doc/zarya
```

User data:

```
~/.config/Zarya/
~/.local/share/Zarya/
~/.cache/Zarya/
```

**systemd future:**

```
/etc/systemd/system/zarya-helper.service
/usr/share/polkit-1/actions/dev.vladon.zarya.helper.policy
```
