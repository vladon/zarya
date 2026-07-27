# Windows portable ZIP layout

```
Zarya-0.10.0-windows-x64-portable/
  Zarya.exe
  portable.flag
  Qt6*.dll
  platforms/
  imageformats/
  styles/
  tls/
  data/
  runtime/
  cores/
    xray/
      README.txt
      geoip.dat
      geosite.dat
  README.md
  LICENSE
  LICENSE.MIT
  LICENSE.GPL-3.0
  COPYING
```

Build with `scripts/package-windows.ps1` after a Release build.
