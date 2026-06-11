# Download Verification

Every beta artifact includes SHA256 checksums.

## Windows PowerShell

```powershell
Get-FileHash .\Zarya-0.29.0-beta-windows-x64-portable.zip -Algorithm SHA256
```

Compare the result with the `.sha256` file or `SHA256SUMS.txt`.

## macOS / Linux

```bash
sha256sum -c SHA256SUMS.txt
```

or:

```bash
shasum -a 256 Zarya-0.29.0-beta-macos-arm64.zip
```

## Signed builds

0.29 beta may still be unsigned.
Checksum verification is required.
