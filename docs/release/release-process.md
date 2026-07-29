# Stable Release Process

1. Set the version and stable channel in `cmake/ZaryaVersion.cmake`.
2. Complete the release notes, regression matrix, and Go / No-Go checklist.
3. Merge the release PR after Windows, Linux, and macOS checks pass.
4. Tag the merge commit and push the tag.
5. Wait for every tag release job.
6. Download all workflow artifacts.
7. Verify checksums, manifests, versions, and archive secret audits.
8. Create or update the GitHub Release with the reviewed release draft.
9. Attach all archives, checksum sidecars, and `SHA256SUMS.txt`.
10. Publish only after the final Go decision.

## 1.5.0 commands

```bash
git tag -a v1.5.0 <merge-commit> -m "Zarya 1.5.0"
git push origin v1.5.0

python scripts/verify-release-artifacts.py \
  --artifact dist/Zarya-1.5.0-windows-x64-portable.zip \
  --expected-version 1.5.0 \
  --stable-release \
  --require-checksum \
  --allow-unsigned

python scripts/audit-release-artifact.py \
  --artifact dist/Zarya-1.5.0-windows-x64-portable.zip
```

Repeat verification and the secret audit for the Linux and macOS archives.
Run `scripts/audit-redaction.py` separately on representative diagnostics and
redacted-backup bundles.
