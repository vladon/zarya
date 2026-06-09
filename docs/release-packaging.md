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
  cores/sing-box/README.txt
  data/
  runtime/
  release-manifest.json
  README.md
  LICENSE
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

- Xray
- sing-box
- Geo data (`geoip.dat`, `geosite.dat`)
- sing-box rule sets

Install cores via **Tools → Core Manager**.

## Smoke tests

```bash
python scripts/smoke-package.py --artifact dist/Zarya-....zip
```

Checks:

- artifact extracts
- executables and translations exist
- `LICENSE` and `release-manifest.json` exist
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
