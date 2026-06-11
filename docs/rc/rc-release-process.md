# RC Release Process

1. Update version to rc.
2. Build artifacts.
3. Run package verification.
4. Run stable regression matrix.
5. Run redaction audit.
6. Review known issues.
7. Create release draft.
8. Attach artifacts and checksums.
9. Mark experimental features clearly.
10. Publish RC.

## Verification commands

```bash
python scripts/verify-release-artifacts.py \
  --artifact dist/Zarya-0.36.0-rc1-windows-x64-portable.zip \
  --expected-version 0.36.0-rc1 \
  --release-candidate \
  --allow-unsigned

python scripts/audit-redaction.py --diagnostics zarya-diagnostics.zip
python scripts/audit-redaction.py --redacted-backup redacted-backup.zarya-backup.zip
```
