# Agent handoff — Windows LCL cutover (2026-08-17)

Read this document together with the repository `AGENTS.md` before continuing
the Windows LCL cutover. This is an operational snapshot, not a replacement for
the stable architecture and security documentation.

## Snapshot

| Item | Current value |
|------|---------------|
| Repository | `vladon/zarya` |
| Trunk | `main` at `6cb4bc6cb14701c843a7eba64a15cc3b21c95dd9` |
| Application version | `1.5.13` on `chore/windows-lcl-cutover`; `1.5.12` on `main` |
| Windows release | Qt until the cutover PR merges; LCL prepared on `chore/windows-lcl-cutover` |
| Working tree before this handoff | Clean and synchronized with `origin/main` |
| LCL package contract | `Zarya.exe` + `Zarya.exe.sha256` only |
| Rollback tag | `windows-qt-final-1.5.12` on `6cb4bc6` (created and pushed) |
| Cutover branch | `chore/windows-lcl-cutover` created from `main` at `6cb4bc6` |
| Self-hosted runners | `zarya-win10-gate` / `zarya-win11-gate`, online |

The old local snapshot branch `wip/lcl-precutover-473fea3` and the merged local
feature branch were deleted after the seven-PR series landed. The remote branch
`chore/lcl-release-engineering` was also deleted. Do not recreate or publish the
old mixed snapshot.

Other local branches/worktrees predate this effort and belong to the user. Do
not delete or rewrite them.

## Update 2026-08-19 — release gate complete

- Both self-hosted runners are registered, online, and idle:
  `zarya-win10-gate` (`windows-10-x64`) and `zarya-win11-gate`
  (`windows-11-x64`).
- The gate ran with `release_gate=true` on frozen `main` at `6cb4bc6`
  (run `32042153728`, 2026-08-17). `hosted-windows`,
  `release-gate (windows-10-x64)`, and `release-gate (windows-11-x64)` all
  passed: build, Pascal/Go tests, the six-core matrix, and exact packaging.
- The manual UI matrix passed on the 1.5.12 LCL artifact: 100%, 150%, and
  200% DPI; EN/RU; light/dark theme; Tab order; keyboard-only operation; and
  Narrator.
- The rollback tag `windows-qt-final-1.5.12` was created and pushed exactly on
  `6cb4bc6` (step 4). The cutover branch bumps to `1.5.13` and implements
  step 6, including removal of the duplicate `hosted-windows` job from
  `lcl-windows.yml` (the main Windows build job is now the LCL flow).

## Landed LCL series

All changes were delivered as scoped squash-merged PRs:

| Version | PR | Merge commit | Result |
|---------|----|--------------|--------|
| 1.5.6 | [#225](https://github.com/vladon/zarya/pull/225) isolated runtime workers | `fb253c7` | ABI v2, typed worker, Real delay, dynamic ports |
| 1.5.7 | [#226](https://github.com/vladon/zarya/pull/226) routing/DNS/geodata | `83b3fcb` | Versioned stores, adapter mappings, atomic geodata |
| 1.5.8 | [#227](https://github.com/vladon/zarya/pull/227) migration/privacy | `24ccdd6` | Qt staging migration, rollback, backup/redaction |
| 1.5.9 | [#228](https://github.com/vladon/zarya/pull/228) Windows lifecycle | `7f7b084` | Settings, autostart, proxy recovery, shutdown |
| 1.5.10 | [#229](https://github.com/vladon/zarya/pull/229) stable UI/i18n | `5aa53e5` | First run, EN/RU, stable gates |
| 1.5.11 | [#230](https://github.com/vladon/zarya/pull/230) application services | `a3256fd` | Runtime/profile/background services split from `MainForm` |
| 1.5.12 | [#231](https://github.com/vladon/zarya/pull/231) release engineering | `6cb4bc6` | Pinned toolchain, known cores, exact package, LCL CI |

PR #231 passed hosted LCL Windows plus the existing Windows Qt, Ubuntu, macOS,
lib_ui-boundary, and release-signing-contract jobs before merge.

## Stable behavior that must not change

- Stable runtime is embedded Xray with system proxy.
- System proxy is enabled only after readiness polling succeeds: every 100 ms,
  at most 5 seconds. Process launch alone is not readiness.
- There is no silent provider fallback. The user must choose one-time use or
  update the profile provider.
- Worker stdout contains only the final typed JSON protocol result; logs go to
  stderr. Temporary Xray tests remain isolated from the main in-process runtime.
- External cores are user-owned files. Zarya and official artifacts do not
  download, back up, or distribute them. CI downloads pinned fixtures only.
- Stable ZIP contains exactly `Zarya.exe` and `Zarya.exe.sha256`.
- Embedded sing-box, TUN, helper/service, kill switch, and self-update remain
  experimental and gated until after the stable cutover.
- Raw configs, credentials, full external paths, user data, and runtime logs
  must not leak into release artifacts or diagnostics.

## Verification already completed

The following were rerun locally on `6cb4bc6` on Windows build 26200 (Windows 11
25H2) and passed:

```powershell
cd D:\projects\zarya\prototypes\lcl
.\build.ps1
.\test.ps1
.\test-known-cores.ps1
.\package.ps1

cd D:\projects\zarya\src\runtime\embedded\xray\bridge
$env:CC='D:\projects\zarya\build\tools\winlibs\mingw64\bin\x86_64-w64-mingw32-gcc.exe'
go test ./...

cd D:\projects\zarya
python tests\release_signing_contract_test.py
git diff --check
```

The real external-core matrix passed with:

- Xray `26.3.27`;
- sing-box `1.13.18`;
- V2Ray `5.52.0`;
- Hysteria2 `2.12.1`;
- Mihomo `1.19.29`;
- `nekobox_core` `5.11.28.2`.

The local unsigned artifact is
`prototypes/lcl/dist/Zarya-1.5.12-windows-x64-portable.zip`. It has exactly two
root entries. The locally built executable SHA-256 was
`017dcb251e6e6c3e28a44804a5b160fed984499203a75d93e7050c776e84574b`.
This hash is only evidence for that local build and is not a release checksum.

An `xray.exe` observed after the tests belongs to the user's long-running
`v2rayN.exe` process, not to Zarya's test matrix. Do not terminate it as test
cleanup.

## Current blockers

> Status 2026-08-19: all three blockers below are resolved — runners are
> online, the gate passed (run `32042153728`), and the UI matrix is complete.
> The text is kept for the record.

### 1. No self-hosted Windows runners

GitHub currently reports zero repository runners. The required workflow matrix
cannot run until two online runners exist with these labels:

- Windows 10: `self-hosted`, `windows`, `x64`, `windows-10-x64`;
- Windows 11: `self-hosted`, `windows`, `x64`, `windows-11-x64`.

Do not dispatch the gate while no matching runners exist; it will only create
indefinitely queued jobs. Do not substitute Windows Server hosted images for
the required Windows 10/11 client systems.

### 2. Manual UI matrix is incomplete

The Computer Use native helper was unavailable (`failed to connect native
pipe: file not found`), so the UI matrix was not claimed as passed. Test the
same LCL artifact at 100%, 150%, and 200% DPI with EN/RU, light/dark theme,
Tab order, keyboard-only operation, and Narrator.

### 3. GitHub CLI context

GitHub authentication is restored in the real user keyring. In a sandboxed
agent shell, plain `gh auth status` may still show the old invalid credential;
the same command with escalated/user context succeeds. Never print the token.

## Exact next steps

1. Confirm both self-hosted runners are online and idle:

   ```powershell
   gh api repos/vladon/zarya/actions/runners --jq `
     '{total_count, runners:[.runners[]|{name,status,busy,labels:[.labels[].name]}]}'
   ```

2. Dispatch and watch the gate on the frozen pre-cutover commit:

   ```powershell
   gh workflow run lcl-windows.yml --ref main -f release_gate=true
   gh run list --workflow "LCL Windows" --limit 3
   gh run watch <run-id> --exit-status
   ```

3. Require both `windows-10-x64` and `windows-11-x64` jobs to pass build,
   Pascal/Go tests, the six-core matrix, exact packaging, and process cleanup.
   Complete and record the manual UI matrix. Any defect goes through a separate
   signed `fix/` PR and a full repeated gate.

4. Only after the full gate, create a signed annotated non-release tag exactly
   on `6cb4bc6` and push it:

   ```powershell
   git -c gpg.format=ssh `
     -c user.signingkey=C:/Users/vladon/.ssh/id_ed25519 `
     tag -s windows-qt-final-1.5.12 6cb4bc6 `
     -m "Final Windows Qt release before LCL cutover"
   git push origin windows-qt-final-1.5.12
   ```

5. Create `chore/windows-lcl-cutover` from current `main`; bump to `1.5.13`.
   Never push directly to `main`.

6. In the cutover PR:

   - switch only Windows jobs in `build.yml` and `release.yml` to the pinned LCL
     build/test/package flow; keep Linux/macOS on Qt/lib_ui;
   - leave `lcl-windows.yml` as the manual self-hosted release gate and remove
     its duplicate hosted job after the main Windows build job becomes LCL;
   - add verifier mode `--windows-lcl-single-exe` requiring exactly the two
     root files, the inner checksum, CLI/PE version, and optionally Authenticode;
   - keep the legacy Qt/Linux/macOS verifier behavior unchanged;
   - make active SignPath configuration sign only `Zarya.exe`;
   - after signing, verify publisher/timestamp, regenerate
     `Zarya.exe.sha256`, and create the exact two-file ZIP;
   - make stable Windows release fail closed if SignPath is disabled or
     misconfigured;
   - add negative package/signing tests and release notes for `1.5.13`.

7. Run the required local checks, push a signed commit, open a draft PR, wait
   for all CI, mark ready, and squash-merge only when green.

8. Create signed product tag `v1.5.13` only after merge. Verify the signed ZIP
   on clean Windows 10/11 before treating cutover as successful.

9. Only after a successful LCL release, create
   `chore/remove-windows-qt` with version `1.5.14`: remove Windows Qt/lib_ui,
   helper/updater/worker targets and obsolete Windows static-Qt packaging while
   preserving Qt/lib_ui on Linux/macOS.

## Cutover release blockers

Do not proceed if any of these remain:

- early system-proxy activation or incomplete WinINet restoration;
- data loss, partial migration, or mixed-version geodata;
- a runtime/test process left behind;
- failed embedded Xray or any supported external adapter;
- raw config, credentials, external executables, user data, or full paths in
  ZIP/backup/diagnostics;
- unsigned/untrusted `Zarya.exe` or a ZIP containing anything except the two
  allowed files.

## Git workflow

- Trunk is `main`; never push to it directly.
- Use one short-lived `feat/`, `fix/`, or `chore/` branch per result.
- Stage only scoped files, create SSH-signed commits, open a draft PR, include a
  test plan, and wait for relevant CI before merge.
- Every merged PR gets the next patch version.
- The handoff itself was requested as a workspace edit; do not assume it has
  been committed. Inspect `git status` before starting new work.

## Prompt for the next session

```text
Read AGENTS.md and docs/agent-handoff-2026-08-17-lcl-cutover.md completely.
The Windows LCL release gate and manual UI matrix are complete (run 32042153728
on main at 6cb4bc6), and the rollback tag windows-qt-final-1.5.12 is pushed.
Continue with the chore/windows-lcl-cutover PR (version 1.5.13): wait for all
CI, mark ready, squash-merge when green, then create the signed tag v1.5.13
and verify the signed ZIP on clean Windows 10/11 before starting
chore/remove-windows-qt (1.5.14). Never push directly to main; use signed
commits and scoped PRs. Do not kill the user's v2rayN-owned xray.exe.
```
