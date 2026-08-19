# Agent handoff — post-cutover follow-up (2026-08-19)

Read this document together with the repository `AGENTS.md` and
`docs/agent-handoff-2026-08-17-lcl-cutover.md` before continuing. This is the
successor snapshot: the Windows LCL cutover itself is **done and released**;
this document covers the optional-signing follow-up and the remaining release
publishing work.

## Snapshot

| Item | Current value |
|------|---------------|
| Trunk | `main` at `ed68ab6` (v1.5.13, PR #232 squash-merged) |
| Tags pushed | `windows-qt-final-1.5.12` → `6cb4bc6`; `v1.5.13` → `ed68ab6` (both SSH-signed) |
| v1.5.13 release run | `32238357476` — success; unsigned artifacts uploaded (`zarya-windows-release`, `zarya-linux-release`, `zarya-macos-release`); `release-windows-sign` skipped because SignPath is not configured |
| GitHub Release | **Not created yet** (workflow never publishes; release-process.md makes it a manual step) |
| In-flight PR | **#233 «Make release code signing optional (1.5.14)»** — draft, branch `chore/optional-release-signing` @ `8883092` (signed `G`), **5/5 checks green**, NOT merged |
| Local branch | `chore/optional-release-signing` checked out, clean; commit `8883092` amended+signed then force-pushed |
| SignPath repo config | None (`gh variable list` / `gh secret list` empty) — signing inactive by design now |
| Self-hosted runners | `zarya-win10-gate` / `zarya-win11-gate` online |

## What was completed this session

1. **Closed the cutover (per the 2026-08-17 handoff):**
   - Verified the 2026-08-17 gate run `32042153728` (both self-hosted Win10/11
     matrix jobs + hosted job) was green on frozen `6cb4bc6`; recorded the
     completed manual UI matrix in the handoff doc (user confirmed it passed).
   - Reduced `.github/workflows/lcl-windows.yml` to the manual self-hosted
     release gate only (removed the duplicate `hosted-windows` job and its
     `pull_request`/`push` triggers — the main Windows build job is now LCL).
   - Re-signed the whole cutover branch with `git rebase --force-rebase
     --gpg-sign origin/main` (11 commits, all `G`).
   - Created and pushed the signed rollback tag `windows-qt-final-1.5.12` on
     `6cb4bc6`.
   - Local verification on Windows 11 25H2: `build.ps1`, `test.ps1`,
     `test-known-cores.ps1` (Xray 26.3.27, sing-box 1.13.18, V2Ray 5.52.0,
     Hysteria2 2.12.1, Mihomo 1.19.29, nekobox-core 5.11.28.2), `package.ps1`,
     Go bridge tests with pinned MinGW `CC`, `release_signing_contract_test.py`
     13/13, strict `--windows-lcl-single-exe` verify OK, secret audit OK.
   - PR #232: CI 5/5 green → ready → squash-merged → `main` = `ed68ab6`,
     version 1.5.13.
   - Extra post-merge self-hosted gate on `ed68ab6` (run `32198337686`) hung
     ~100 min on cold toolchain bootstrap; user chose to cancel and proceed.
   - Created and pushed signed tag `v1.5.13` → release run `32238357476`
     success (unsigned, SignPath not configured — `release-windows-sign`
     skipped). Downloaded and verified the Windows artifact locally:
     `Verification OK`, secret audit OK, ZIP = exactly `Zarya.exe` +
     `Zarya.exe.sha256`, inner hash matches, PE 1.5.13, CLI `Zarya 1.5.13`.

2. **Optional signing decision (user: «давай пока уберем требование signing
   совсем»):** branch `chore/optional-release-signing`, commit `8883092`
   «chore: make release code signing optional (1.5.14)» (13 files, +74/−30):
   - Docs: signing is optional at every release level incl. stable; stable
     releases are unsigned with mandatory SHA-256 checksums. Updated
     `docs/signing/README.md`, `code-signing-policy.md`,
     `release-verification.md`, `docs/release-packaging.md`,
     `docs/public-beta/download-verification.md`, LCL `README.md`/`MIGRATION.md`,
     and the 2026-08-17 handoff (fail-closed-if-disabled marked superseded;
     unsigned no longer a cutover blocker — checksums mandatory).
   - Stale Qt-era signed-binary lists now name only `Zarya.exe`.
   - Version bumped to 1.5.14 in lockstep (CMake, `ZaryaVersion.pas`, LPI/PE,
     CLI); `docs/release-notes/1.5.14.md` + `RELEASE_NOTES.md` entry.
   - **No CI/workflow code changes**: SignPath path stays dormant unless
     `SIGNPATH_ENABLED=true`; `--require-signed` remains for that optional
     path. Contract tests unchanged and passing.
   - Verified locally: rebuild + `Zarya.exe --version` → `Zarya 1.5.14`;
     `test.ps1` all PASS incl. `CLI, Pascal and CMake version contract`;
     `release_signing_contract_test.py` 13/13 OK; `git diff --check` clean.
   - PR #233 draft opened; checks: build-windows 3m59s ✅, build-macos 15m4s ✅,
     build-ubuntu 53m54s ✅, libui-boundary ✅, release-signing-contract ✅.

## Exact next steps

1. Finish PR #233 (all checks already green):

   ```powershell
   gh pr ready 233
   gh pr merge 233 --squash --delete-branch
   git checkout main; git pull --ff-only
   ```

2. Tag v1.5.14 (see signing workaround below) and push; watch the release run:

   ```powershell
   git -c gpg.format=ssh -c user.signingkey=<key-copy> tag -s v1.5.14 <merge-commit> -m "Zarya 1.5.14 — optional release signing"
   git push origin v1.5.14
   gh run list --workflow Release --limit 1
   ```

   With signing optional, `release-windows-sign` will skip (no SignPath
   variables) and `zarya-windows-release` will be the unsigned LCL ZIP — this
   is now the intended outcome, not a failure.

3. Verify the artifact (same as done for 1.5.13):

   ```powershell
   gh run download <run-id> -n zarya-windows-release -D D:\scratch\zarya-1.5.14
   python scripts\verify-release-artifacts.py --artifact <zip> --expected-version 1.5.14 --windows-lcl-single-exe --require-checksum --allow-unsigned
   python scripts\audit-release-artifact.py --artifact <zip>
   cmd /c "<extracted>\Zarya.exe --version"   # GUI-subsystem: use cmd for stdout
   ```

4. Publish the GitHub Release manually (workflow never does): create a release
   on tag `v1.5.14` from `docs/release-notes/1.5.14.md`, attach the three
   platform artifacts + `.sha256` + `SHA256SUMS.txt` from the run, publish
   after the Go decision (`docs/stable/go-no-go-checklist.md`; signing boxes no
   longer block — checksums mandatory).

5. Optional, when desired: re-run the self-hosted gate
   (`gh workflow run lcl-windows.yml --ref main -f release_gate=true`) **after**
   cleaning `<runner-workspace>\build\tools\lcl-toolchain\downloads\` on both
   runner machines — the cancelled run left partial downloads that poison the
   next bootstrap with "Cached download hash mismatch". Consider a separate
   `fix/` PR adding timeouts/caching to `bootstrap-toolchain.ps1`.

6. Only after a successful 1.5.14 release: start `chore/remove-windows-qt`
   (version 1.5.15; the 2026-08-17 handoff called it 1.5.14 before the
   optional-signing PR took that number) — remove Windows Qt/lib_ui,
   helper/updater/worker targets and obsolete Windows static-Qt packaging
   while preserving Qt/lib_ui on Linux/macOS.

## SSH commit/tag signing from the agent sandbox

`C:/Users/vladon/.ssh/id_ed25519` has an ACL entry for `CodexSandboxUsers`, so
ssh refuses it directly. Workaround used (no passphrase on the key):

```powershell
$k = Join-Path $env:TEMP 'zarya-sign-key'
Copy-Item C:/Users/vladon/.ssh/id_ed25519 $k -Force
icacls $k /inheritance:r | Out-Null
icacls $k /grant:r "$($env:USERNAME):R" | Out-Null
# commit: needs -c commit.gpgsign=true (plain -c user.signingkey alone does NOT sign!)
git -c gpg.format=ssh -c user.signingkey=$k -c commit.gpgsign=true commit ...
# tag:
git -c gpg.format=ssh -c user.signingkey=$k tag -s <tag> <sha> -m "..."
# verify with allowed-signers, then delete the temp copy
```

Key fingerprint seen in good signatures:
`ED25519 SHA256:UjWW2ozOR+Xg+sR5Gm7nCyt940amOeMQQCkI3aqtVcU`.

## Operational notes

- Never kill the user's `xray.exe` (owned by long-running `v2rayN.exe`).
- `gh` works in the agent shell; `gh pr create` may race right after a branch
  push ("Head sha can't be blank") — retry after ~15 s, use `--body-file`.
- The extra gate run `32198337686` was cancelled after a ~100 min hang on
  "Bootstrap pinned LCL toolchain" (cold download, no timeouts in the script).
- Self-hosted gate has no `actions/cache`; hosted jobs do (key
  `lcl-4.8-fpc-3.2.2-winlibs-16.1.0-r2`).
- Working copy currently on `chore/optional-release-signing`; this handoff file
  is **untracked/uncommitted** — fold it into the next PR (e.g.,
  `chore/remove-windows-qt`) or a docs commit after merging #233.

## Prompt for the next session

```text
Read AGENTS.md, docs/agent-handoff-2026-08-19-post-cutover.md, and
docs/agent-handoff-2026-08-17-lcl-cutover.md. The Windows LCL cutover is done
(main ed68ab6 = 1.5.13, tags windows-qt-final-1.5.12 and v1.5.13 pushed).
Continue: PR #233 (optional signing, 1.5.14) is draft with 5/5 green checks —
mark ready, squash-merge, tag v1.5.14 signed, wait for the Release run, verify
the unsigned Windows artifact (--windows-lcl-single-exe --require-checksum
--allow-unsigned), then create the GitHub Release manually with all platform
artifacts and checksums. After that start chore/remove-windows-qt (1.5.15).
Never push directly to main; use SSH-signed commits (sandbox ACL workaround in
the handoff). Do not kill the user's v2rayN-owned xray.exe. Clean
build\tools\lcl-toolchain\downloads on both self-hosted runners before
re-running the release gate.
```

