# Release Verification

## Checksums (required for beta)

Every release artifact has:

- per-artifact `*.sha256` sidecar
- aggregated `SHA256SUMS.txt`

```bash
sha256sum -c SHA256SUMS.txt
```

## Automated verification

```bash
python scripts/verify-release-artifacts.py \
  --artifact dist/Zarya-1.5.0-windows-x64-portable.zip \
  --expected-version 1.5.0 \
  --require-checksum \
  --allow-unsigned
```

### Modes

| Flag | Behavior |
|------|----------|
| `--require-checksum` | Fail if SHA256 missing or mismatched |
| `--allow-unsigned` | Accept unsigned artifacts while signing is unavailable |
| `--require-signed` | Fail unless manifest and platform signatures validate |

For Windows, `--require-signed` requires a valid, timestamped Authenticode
signature on `Zarya.exe` (the single-EXE LCL contract; helper, updater, worker,
and bridge binaries are no longer part of the Windows package). It is used by
the optional SignPath finalize path; while signing is inactive, stable
verification runs with `--allow-unsigned` and relies on the mandatory SHA-256
checksums.

## Manifest fields

`release-manifest.json` inside each artifact includes a `signing` block:

```json
{
  "signing": {
    "signed": false,
    "signatureType": null,
    "notarized": false,
    "timestamped": false
  }
}
```

## In-app integrity

Packaged builds include `build-integrity.json` beside the executable (or in app Resources on macOS). The About dialog shows signed/unsigned status.
