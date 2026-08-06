# Windows Authenticode Signing

## What to sign

- `Zarya.exe`
- `zarya-helper.exe`
- future installer exe/msi if any

## What not to sign

- user data
- downloaded Xray/sing-box cores
- runtime configs

## Tooling

- `signtool.exe` from Windows SDK

## Required inputs

- approved SignPath Foundation project for official releases, or a local code-signing certificate
- SignPath protected environment/API token, or a hardware token/HSM for local signing
- timestamp server URL

The official tag workflow submits only GitHub-hosted build artifacts to SignPath,
requires origin verification and manual approval, then validates all three returned
executables and the embedded Xray DLL before publication. The SignPath private key is never available to
GitHub Actions.

## Example

```powershell
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a Zarya.exe
```

Or use the project hook:

```powershell
.\scripts\sign-windows.ps1 -File .\dist\...\Zarya.exe -CertificateThumbprint <thumbprint> -Verify
.\scripts\package-windows.ps1 -Sign -SigningIdentity <thumbprint>
```

## Verification

```powershell
signtool verify /pa /v Zarya.exe
```

```powershell
python scripts\verify-release-artifacts.py --artifact dist\Zarya-....zip --require-signed
```

## Notes

- Portable ZIP itself is not Authenticode-signed.
- Executables inside the ZIP are signed.
- Checksums still generated for the full archive.
