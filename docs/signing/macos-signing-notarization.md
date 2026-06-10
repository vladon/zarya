# macOS Signing and Notarization

## What to sign

- `Zarya.app`
- main executable
- `zarya-helper`
- nested frameworks/libraries

## Tooling

- `codesign`
- `notarytool`
- `stapler`

## Required inputs

- Apple Developer account
- Developer ID Application certificate
- app-specific password or API key for notarization
- hardened runtime settings

## Example flow

```bash
codesign --deep --force --options runtime --sign "Developer ID Application: ..." Zarya.app
ditto -c -k --keepParent Zarya.app Zarya.zip
xcrun notarytool submit Zarya.zip --wait ...
xcrun stapler staple Zarya.app
```

Project hook:

```bash
./scripts/sign-macos.sh --app-path build/zarya.app --identity "Developer ID Application: ..."
./scripts/package-macos.sh --sign --signing-identity "Developer ID Application: ..."
```

## Verification

```bash
codesign --verify --deep --strict --verbose=2 Zarya.app
spctl --assess --type execute --verbose=4 Zarya.app
```

## 0.27 limitation

This milestone adds hooks only. Real notarization remains optional.
