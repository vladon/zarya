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

## Windows portable ZIP (LCL single-EXE cutover contract)

Since the Windows LCL cutover, Windows release artifacts are built from the
pinned LCL toolchain, not from Qt/lib_ui:

```powershell
.\prototypes\lcl\build.ps1
.\prototypes\lcl\test.ps1
.\prototypes\lcl\package.ps1
```

Layout inside the ZIP (exactly two files at the root, no nested directories,
no DLLs):

```
Zarya.exe
Zarya.exe.sha256
```

Verification enforces the strict contract:

```powershell
python scripts\verify-release-artifacts.py `
  --artifact prototypes/lcl/dist/Zarya-<version>-windows-x64-portable.zip `
  --expected-version <version> `
  --windows-lcl-single-exe --require-checksum --allow-unsigned
```

Add `--require-signed` (used by `scripts/finalize-signpath-windows.ps1`) to
fail closed unless `Zarya.exe` carries a valid, timestamped Authenticode
signature. The active SignPath configuration signs only `Zarya.exe`; helper,
updater, worker, and bridge binaries are no longer part of the Windows
package.

The legacy Qt layout below applies to Windows artifacts built before the
cutover (tag `windows-qt-final-1.5.12`) and to Linux/macOS packages:

```
Zarya-<version>-windows-x64-portable/
  Zarya.exe
  zarya-helper.exe
  zarya-core-test-worker.exe
  zarya-xray.dll
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

Static Qt builds of that legacy layout ship a single `Zarya.exe` without separate Qt DLLs.


## macOS app bundle

```bash
./scripts/package-macos.sh --output-dir dist
```

Produces `Zarya-<version>-macos-<arch>.zip` containing `Zarya.app`.

**Unsigned beta:** this build is not signed or notarized. macOS may show a security warning on first launch.

`zarya-helper` is copied into `Contents/MacOS/`. Translations and docs go under `Contents/Resources/`.

Packaging works from a temporary copy of the CMake app bundle so `macdeployqt` cannot contaminate
the local build tree. The package step also rejects missing Cocoa plug-ins and absolute dynamic-library
dependencies outside macOS system locations.

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

## Embedded Xray

Release builds compile the pinned vendored source described by
`third_party/xray-core.zarya.json`. Windows/Linux packages contain a signed companion
library; macOS statically links the Go archive. Packages contain no `xray.exe`.

The release manifest reports `included.xrayDistribution=embedded`, the embedded tag,
commit and ABI version, and the bridge checksum when a companion file exists. Package
verification rejects a missing bridge or an unexpected Xray executable.

## Bundled geo data (runetfreedom)

Release packaging also downloads **pinned** `geoip.dat` / `geosite.dat` from runetfreedom into the same `cores/xray/` directory (see `packaging/geodata/runetfreedom-pin.json`). Override with `ZARYA_BUNDLE_GEODATA=0`, `-SkipBundleGeodata`, or `--skip-bundle-geodata`.

Users can still replace files via **Tools → Geo Data Manager** (any built-in source).

`release-manifest.json` reports `included.geoData`, `included.geoDataSource`, and `included.geoDataVersion`.

Update built-in Xray through **Zarya App Update**. Core Manager continues to manage sing-box; Geo updates remain available in Geo Data Manager.

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
