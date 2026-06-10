# Linux Artifact Signing

## Required baseline

- `SHA256SUMS.txt`

## Optional signatures

- GPG detached signatures
- minisign signatures

## What to sign

- release tarball
- `SHA256SUMS.txt` (optional, for distribution mirrors)

## Examples

```bash
gpg --detach-sign --armor Zarya-...tar.gz
minisign -Sm Zarya-...tar.gz
```

Project hooks:

```bash
./scripts/sign-linux.sh --artifact dist/Zarya-....tar.gz --gpg --key <key-id>
./scripts/package-linux.sh --sign --gpg-sign --key <key-id>
```

## Verification

```bash
sha256sum -c SHA256SUMS.txt
gpg --verify Zarya-...tar.gz.asc Zarya-...tar.gz
minisign -Vm Zarya-...tar.gz -P <public-key>
```
