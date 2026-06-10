# Zarya Beta Bug Triage

Use this document when triaging issues found during the 0.26 beta pass or reported by testers.

## Severity levels

### Critical

Ship-blocking. Fix before publishing a beta artifact, or block the affected path from release.

- App cannot start or crashes on launch
- Data loss or corruption of profiles, subscriptions, or settings
- Backup/import corrupts or silently overwrites state without pre-import backup
- System proxy not restored after Stop, Exit, or tray shutdown on a supported platform
- Kill switch cannot be disabled or recovered; networking remains blocked
- Helper starts an arbitrary binary or accepts paths outside policy
- Diagnostics bundle leaks secrets (share links, UUIDs, passwords, `helper.token`)
- Release artifact missing executable, manifest, or translations

### High

Major broken flow. Fix before beta unless the path is explicitly hidden or marked experimental.

- Core cannot start for a valid, supported profile
- Subscriptions cannot update or delete profiles on failed fetch
- Core Manager cannot install or detect a core
- Migration fails on an older config without a clear recovery path
- First-run wizard dead end (Finish with no path forward)
- Major platform-specific crash in a core flow (import, start, stop)

### Medium

Usable but confusing or incomplete.

- Error message does not explain what to do next
- Missing translation in a primary flow
- Non-critical dialog bug (wrong default, stale status)
- Status indicator not refreshed after an operation
- Experimental feature warning is misleading

### Low

Cosmetic or minor copy issue.

- Layout alignment, spacing, or truncation
- Minor wording inconsistency
- Untranslated deep settings label
- Log line formatting

### Won't fix before beta

Tracked but intentionally deferred.

- New feature requests
- macOS kill switch (unsupported by design)
- Code signing / notarization
- App self-update
- Production service installer
- UI redesign

## Triage workflow

1. Reproduce on a clean portable artifact when possible.
2. Assign severity using definitions above.
3. Classify **Area** (see regression checklist sections).
4. Attach a diagnostics bundle (Help → Create Diagnostics Bundle).
5. Record **Fix status**: Open / In progress / Fixed / Won't fix / Blocked.
6. For Critical/High: link to checklist item and platform matrix row.

## Bug template

Copy into your issue tracker or `docs/beta-issues/` notes.

```markdown
## ID

ZARYA-###

## Severity

Critical | High | Medium | Low | Won't fix before beta

## Area

Fresh start | Core Manager | Profile import | Subscription update | Xray runtime |
System proxy | Routing/DNS | Geo Data | Node testing | Tray/safe shutdown |
Startup recovery | Backup/import | Diagnostics | Localization | Packaging |
TUN experimental | Helper | Kill switch

## Platform

Windows 10 | Windows 11 | macOS arm64 | macOS Intel | Ubuntu GNOME | KDE Plasma | Other

## Build

Zarya 0.26.0-beta (commit ______, artifact path if portable)

## Steps to reproduce

1.
2.
3.

## Expected

## Actual

## Logs/diagnostics

- Diagnostics bundle attached: yes/no
- Relevant log excerpt (redact secrets):
- System proxy state before/after (if applicable):

## Fix status

Open | In progress | Fixed | Won't fix | Blocked
```

## Reporting issues (testers)

1. Use **Help → Create Diagnostics Bundle** with default **Strict** redaction.
2. Review the archive before sending; switch to Basic only if support asks for hostnames.
3. Include exact build version (`Zarya --version` or About dialog).
4. Do not paste raw `vless://`, `vmess://`, `trojan://`, or `ss://` links in public tickets.
