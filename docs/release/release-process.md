# Stable Release Process

1. Update version to stable (1.0.0).
2. Build artifacts.
3. Run package verification.
4. Run stable regression matrix.
5. Run redaction audit.
6. Review known issues.
7. Create release draft.
8. Attach artifacts and checksums.
9. Mark experimental features clearly.
10. Publish stable release.

## Verification commands

```bash
python scripts/verify-release-artifacts.py \
  --artifact dist/Zarya-1.0.0-windows-x64-portable.zip \
  --expected-version 1.0.0 \
  --release-stable \
  --allow-unsigned

python scripts/audit-redaction.py --diagnostics zarya-diagnostics.zip
python scripts/audit-redaction.py --redacted-backup redacted-backup.zarya-backup.zip
```
