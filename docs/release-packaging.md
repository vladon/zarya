# Release Packaging

Zarya beta releases are built with platform scripts under `scripts/`. Version metadata comes from `cmake/ZaryaVersion.cmake` and `BuildInfo`.

## Artifact names

| Platform | Artifact |
|----------|----------|
| Windows portable | `Zarya-<version>-windows-x64-portable.zip` |
| macOS | `Zarya-<version>-macos-<arch>.zip` |
| Linux | `Zarya-<version>-linux-<arch>.tar.gz` |

Example: `Zarya-0.25.0-beta-windows-x64-portable.zip`

Checksum sidecars:

- `<artifact>.sha256`
- `SHA256SUMS.txt` in the output directory

## Windows portable ZIP

```powershell
.\scripts\package-windows.ps1 -Configuration Release -OutputDir .\dist
.\scripts\smoke-windows.ps1 -ArtifactDir .\dist
```

Layout inside the ZIP:

```
Zarya-<version>-windows-x64-portable/
  Zarya.exe
  zarya-helper.exe
  portable.flag
  translations/
  docs/
  cores/xray/README.txt
  cores/xray/geoip.dat
  cores/xray/geosite.dat
  cores/sing-box/README.txt
  data/
  runtime/
  release-manifest.json
  README.md
  LICENSE
  LICENSE.MIT
  LICENSE.GPL-3.0
  COPYING
  THIRD_PARTY_NOTICES.md
  RELEASE_NOTES.md
```

`windeployqt` is used when available (shared Qt builds). Static Qt builds ship a single `Zarya.exe` without separate Qt DLLs.

## macOS app bundle

```bash
./scripts/package-macos.sh --output-dir dist
```

Produces `Zarya-<version>-macos-<arch>.zip` containing `Zarya.app`.

**Unsigned beta:** this build is not signed or notarized. macOS may show a security warning on first launch.

`zarya-helper` is copied into `Contents/MacOS/`. Translations and docs go under `Contents/Resources/`.

## Linux tarball

```bash
./scripts/package-linux.sh --output-dir dist
./scripts/smoke-linux.sh --artifact-dir dist
```

Includes `zarya`, `zarya-helper`, `zarya.desktop`, `portable.flag`, translations, docs, and placeholder directories.

Shared-Qt tarballs may require Qt 6 runtime libraries on the target system unless you bundle Qt manually.

### AppImage skeleton

`packaging/linux/AppDir/` contains a minimal AppDir skeleton (`AppRun`, `.desktop`, icon placeholder). Full AppImage generation is not required for 0.25.

## Checksums

After packaging:

```powershell
Get-FileHash .\dist\Zarya-0.25.0-beta-windows-x64-portable.zip -Algorithm SHA256
```

```bash
sha256sum -c SHA256SUMS.txt
```

## What is not bundled

- sing-box
- sing-box rule sets

## Bundled Xray seed

Release packaging downloads a **pinned** Xray binary into `cores/xray/` (see `packaging/cores/xray-pin.json`) so first launch works offline. Override with `ZARYA_BUNDLE_XRAY=0`, `-SkipBundleXray`, or `--skip-bundle-xray`.

Users can still:

- replace the managed binary via **Tools → Core Manager**
- point **Settings → Xray executable** at an external path

`release-manifest.json` reports `included.xray`, `included.xrayVersion`, and `included.xraySource`.

## Bundled geo data (runetfreedom)

Release packaging also downloads **pinned** `geoip.dat` / `geosite.dat` from runetfreedom into the same `cores/xray/` directory (see `packaging/geodata/runetfreedom-pin.json`). Override with `ZARYA_BUNDLE_GEODATA=0`, `-SkipBundleGeodata`, or `--skip-bundle-geodata`.

Users can still replace files via **Tools → Geo Data Manager** (any built-in source).

`release-manifest.json` reports `included.geoData`, `included.geoDataSource`, and `included.geoDataVersion`.

Install/update cores via **Tools → Core Manager**. Geo updates remain available in Geo Data Manager.

## Smoke tests

```bash
python scripts/smoke-package.py --artifact dist/Zarya-....zip
```

Checks:

- artifact extracts
- executables and translations exist
- `LICENSE` (and companion `LICENSE.MIT` / `LICENSE.GPL-3.0` / `COPYING`) and `release-manifest.json` exist
- forbidden user/runtime files are absent
- `--version` works on the current OS

## Script options

Common flags:

- `--output-dir` / `-OutputDir`
- `--build-dir` / `-BuildDir`
- `--skip-build`
- `--skip-tests`

## Known packaging limitations

- No code signing or notarization
- Linux Qt bundling depends on build type
- AppImage is skeleton only
- Version commit in manifest uses `git rev-parse` at package time
