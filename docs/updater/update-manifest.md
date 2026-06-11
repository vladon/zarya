# Update manifest format

Zarya uses a standalone `update-manifest.json` distinct from `release-manifest.json` (which describes one artifact's contents).

## Fields

| Field | Description |
|-------|-------------|
| `format` | Must be `zarya-update-manifest` |
| `formatVersion` | Currently `1` |
| `generatedAt` | ISO-8601 UTC timestamp |
| `channels` | Map of channel name → channel entry |

### Channel entry

| Field | Description |
|-------|-------------|
| `latestVersion` | Newest version on this channel |
| `minSupportedVersion` | Oldest version that can update through this manifest |
| `releaseNotesUrl` | Optional URL for release notes |
| `mandatory` | Future use; not enforced in 0.32 |
| `assets` | Platform-specific download artifacts |

### Asset

| Field | Description |
|-------|-------------|
| `platform` | `windows`, `macos`, `linux` |
| `architecture` | `x64`, `arm64` |
| `installationMode` | `portable` or `installed` |
| `fileName` | Artifact file name |
| `url` | Download URL or `file://` path |
| `sizeBytes` | Expected size |
| `sha256` | SHA-256 hex digest |
| `signature` | Future detached signature (`type`, `url`) |

## Sample manifest

```json
{
  "format": "zarya-update-manifest",
  "formatVersion": 1,
  "generatedAt": "2026-06-08T12:00:00Z",
  "channels": {
    "beta": {
      "latestVersion": "0.33.0-beta",
      "minSupportedVersion": "0.30.0-beta",
      "releaseNotesUrl": "https://example.invalid/releases/0.33.0-beta",
      "mandatory": false,
      "assets": [
        {
          "platform": "windows",
          "architecture": "x64",
          "installationMode": "portable",
          "fileName": "Zarya-0.33.0-beta-windows-x64-portable.zip",
          "url": "https://example.invalid/Zarya-0.33.0-beta-windows-x64-portable.zip",
          "sizeBytes": 12345678,
          "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
          "signature": {
            "type": "none",
            "url": null
          }
        }
      ]
    }
  }
}
```

Generate manifests from build artifacts:

```bash
python scripts/generate-update-manifest.py \
  --version 0.33.0-beta \
  --channel beta \
  --dist-dir dist \
  --base-url file:///absolute/path/to/dist \
  --output update-manifest.json
```
