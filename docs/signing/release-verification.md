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
  --artifact dist/Zarya-0.27.0-beta-windows-x64-portable.zip \
  --expected-version 0.27.0-beta \
  --require-checksum \
  --allow-unsigned
```

### Modes

| Flag | Behavior |
|------|----------|
| `--require-checksum` | Fail if SHA256 missing or mismatched |
| `--allow-unsigned` | Accept unsigned artifacts (0.27 default) |
| `--require-signed` | Fail unless manifest and platform signatures validate |

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
