# Silent Startup Recovery

Date: 2026-07-27  
Status: approved design

## Goal

Remove the modal **Startup Recovery** dialog on GUI startup. Keep the same recovery actions, applied automatically with the previous dialog defaults, and surface results in the UI log.

## Problem

After an unclean shutdown, leftover runtime markers (system proxy snapshot / temp configs / kill-switch marker) trigger `StartupRecoveryDialog`. The dialog is interruptive. Choosing **Skip** does not clear temp configs or mark a clean shutdown, so the dialog can reappear on later launches.

## Decision

**Silent auto-recover (approach A):** on startup, if recovery items are detected, apply the recommended plan immediately without a modal. Do not add a settings toggle. Do not keep a soft/hard split that still shows the dialog for kill switch.

## Behavior

1. `MainWindow::runStartupRecovery()` still runs early in startup.
2. Call `StartupRecovery::detect()`.
3. If nothing is detected (`!uncleanShutdown` / empty `detectedLines`), return.
4. Otherwise call `StartupRecovery::apply(plan, …)` using the plan from `detect()` (same defaults as accepting Recover with all relevant checkboxes checked):
   - restore system proxy when `systemProxyMayBeEnabled`
   - clean runtime temp configs when present (`config-xray.json`, `config-singbox.json`, `sing-box-tun.json`)
   - recover kill switch when marker present
   - `markCleanShutdown()` when unclean shutdown was detected
5. Append detection + apply log lines to the UI log (existing `appendLog` path). No toast, no modal, no diagnostics/reporting shortcuts from this flow.

Manual recovery remains available via existing Settings / helper CLI paths. `AppController::recoverPreviousSession()` already applies silently and stays as-is.

## Code changes

| Area | Change |
|------|--------|
| `MainWindow::runStartupRecovery()` | detect → apply → log; remove dialog |
| `StartupRecoveryDialog` (.h/.cpp) | delete |
| `CMakeLists.txt` | drop dialog source |
| `translations/zarya_en.ts`, `zarya_ru.ts` | remove `StartupRecoveryDialog` context |
| `docs/recovery.md` | document silent startup recovery instead of the modal |

No changes to `StartupRecovery::detect()` / `apply()` logic unless a small helper is needed to share logging with `AppController` (optional; not required).

## Out of scope

- Changing detection heuristics (when proxy/temp/kill-switch count as unclean)
- New settings UI for recovery preferences
- Changing kill-switch / helper recovery CLI
- Fixing root causes of unclean shutdown markers (separate work)

## Success criteria

- After a crash/kill that leaves proxy and/or runtime temp files, next GUI start shows **no** Startup Recovery window.
- Recommended recovery actions still run; UI log records what was detected and applied.
- Kill-switch marker still triggers silent recover (same as previous default checkbox).
- Stable release criteria “unclean shutdown recovery works” remains true (actions happen; interaction is silent).

## Test plan

1. Leave `runtime/config-xray.json` (or sing-box equivalent) on disk; start app → no dialog; file removed; log mentions cleanup.
2. With `restoreProxyOnExit` and a persisted previous proxy / Zarya-owned proxy leftover → start → no dialog; proxy restore attempted; log line present.
3. With kill-switch marker present → start → no dialog; recover attempted (may need elevation as today).
4. Clean shutdown path → start → no recovery log noise / no dialog.
