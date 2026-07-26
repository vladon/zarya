# Silent Startup Recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** On unclean shutdown, apply startup recovery silently (no modal) and log results in the UI.

**Architecture:** Keep `StartupRecovery::detect()` / `apply()` unchanged. Change `MainWindow::runStartupRecovery()` to apply the detected plan immediately, then delete `StartupRecoveryDialog` and update docs/i18n/CMake.

**Tech Stack:** Qt 6 / C++20, CMake, `translations/zarya_{en,ru}.ts`

## Global Constraints

- Namespace: `zarya`
- Do not change `StartupRecovery::detect()` / `apply()` heuristics unless required to compile
- No new settings toggle for the dialog
- Manual recovery via Settings / helper CLI stays available
- User-facing EN+RU: remove obsolete dialog strings; no new user-facing strings required (log lines stay English `QStringLiteral` like today)
- Prefer minimal diff; do not commit unless the user asks (plan commit steps are optional checkpoints)

## File map

| File | Role |
|------|------|
| `src/ui/MainWindow.cpp` | Silent detect → apply → `appendLog` |
| `src/ui/StartupRecoveryDialog.h` | Delete |
| `src/ui/StartupRecoveryDialog.cpp` | Delete |
| `CMakeLists.txt` | Drop dialog source from `zarya` target |
| `translations/zarya_en.ts` | Remove `StartupRecoveryDialog` context |
| `translations/zarya_ru.ts` | Remove `StartupRecoveryDialog` context |
| `docs/recovery.md` | Document silent startup recovery |
| `docs/superpowers/specs/2026-07-27-silent-startup-recovery-design.md` | Spec (already written; include in PR) |

No new files.

---

### Task 1: Silent `MainWindow::runStartupRecovery`

**Files:**
- Modify: `src/ui/MainWindow.cpp` (include + `runStartupRecovery`)
- Unchanged: `src/ui/MainWindow.h` (`void runStartupRecovery();` stays)

**Interfaces:**
- Consumes: `StartupRecovery::detect()` → `StartupRecoveryPlan`; `StartupRecovery::apply(const StartupRecoveryPlan&, QStringList*, QString*)` → `bool`
- Produces: same early-startup call site; no dialog side effects

- [ ] **Step 1: Confirm current dialog wiring (baseline)**

Open `src/ui/MainWindow.cpp` around `runStartupRecovery` and confirm it still constructs `StartupRecoveryDialog` and calls `dialog.exec()`.

- [ ] **Step 2: Replace `runStartupRecovery` with silent apply**

Remove `#include "ui/StartupRecoveryDialog.h"` from `MainWindow.cpp`.

Replace the body of `MainWindow::runStartupRecovery()` with:

```cpp
void MainWindow::runStartupRecovery()
{
    const StartupRecoveryPlan plan = StartupRecovery::detect();
    if (!plan.uncleanShutdown && plan.detectedLines.isEmpty()) {
        return;
    }

    appendLog(QStringLiteral("Unclean previous shutdown detected."));
    for (const QString& line : plan.detectedLines) {
        appendLog(QStringLiteral("Recovery detected: %1").arg(line));
    }

    QStringList logLines;
    if (!StartupRecovery::apply(plan, &logLines)) {
        appendLog(QStringLiteral("Startup recovery encountered errors."));
    }
    for (const QString& line : logLines) {
        appendLog(line);
    }
}
```

Notes:
- Pass `plan` from `detect()` directly (defaults already match previous Recover checkboxes: `restoreSystemProxy=true`, `cleanRuntimeTempFiles=true`, `disableKillSwitch` set in `detect()` when marker present).
- Do **not** keep Skip / diagnostics / reporting connections.
- Keep `#include "recovery/StartupRecovery.h"`.
- Keep `#include "packaging/PublicBetaDocs.h"` — still used by Help menu actions.

- [ ] **Step 3: Grep for leftover dialog usage in MainWindow**

Run:

```powershell
rg "StartupRecoveryDialog" src/ui/MainWindow.cpp src/ui/MainWindow.h
```

Expected: no matches.

- [ ] **Step 4: Commit (only if user asked to commit)**

```powershell
git add src/ui/MainWindow.cpp
git commit -m "fix: apply startup recovery silently without a dialog"
```

---

### Task 2: Remove `StartupRecoveryDialog` from the build

**Files:**
- Delete: `src/ui/StartupRecoveryDialog.h`
- Delete: `src/ui/StartupRecoveryDialog.cpp`
- Modify: `CMakeLists.txt` (remove the dialog source line)

**Interfaces:**
- Consumes: Task 1 removed all call sites
- Produces: `zarya` target builds without dialog sources

- [ ] **Step 1: Confirm no remaining references**

Run:

```powershell
rg "StartupRecoveryDialog" src CMakeLists.txt
```

Expected before delete: `CMakeLists.txt` line with `src/ui/StartupRecoveryDialog.cpp`, plus the two dialog source files. No other `src/` references after Task 1.

- [ ] **Step 2: Remove from CMake**

In `CMakeLists.txt`, delete this line from the `zarya` sources list:

```cmake
    src/ui/StartupRecoveryDialog.cpp
```

Leave neighboring dialog entries (`DiagnosticsPreviewDialog.cpp`, `ReadinessDialog.cpp`, etc.) untouched.

- [ ] **Step 3: Delete dialog sources**

Delete:

- `src/ui/StartupRecoveryDialog.h`
- `src/ui/StartupRecoveryDialog.cpp`

- [ ] **Step 4: Build `zarya`**

Run:

```powershell
cmake --build build --config Release --target zarya
```

Expected: build succeeds; no missing-include / unresolved `StartupRecoveryDialog` errors.

- [ ] **Step 5: Commit (only if user asked to commit)**

```powershell
git add CMakeLists.txt
git add -u src/ui/StartupRecoveryDialog.h src/ui/StartupRecoveryDialog.cpp
git commit -m "chore: remove unused StartupRecoveryDialog"
```

---

### Task 3: Translations and recovery docs

**Files:**
- Modify: `translations/zarya_en.ts`
- Modify: `translations/zarya_ru.ts`
- Modify: `docs/recovery.md`

**Interfaces:**
- Consumes: dialog removed (no `tr()` contexts left for it)
- Produces: docs match silent behavior; translation files have no dead `StartupRecoveryDialog` context

- [ ] **Step 1: Remove EN translation context**

In `translations/zarya_en.ts`, delete the entire block:

```xml
<context>
    <name>StartupRecoveryDialog</name>
    ...
</context>
```

(from `<context>` with `StartupRecoveryDialog` through its closing `</context>`, immediately before `<context><name>StatusDashboardWidget</name>`).

- [ ] **Step 2: Remove RU translation context**

In `translations/zarya_ru.ts`, delete the matching `<context><name>StartupRecoveryDialog</name>…</context>` block the same way.

- [ ] **Step 3: Grep translations**

Run:

```powershell
rg "StartupRecoveryDialog" translations
```

Expected: no matches.

- [ ] **Step 4: Update `docs/recovery.md` unclean-shutdown section**

Replace the top section (through the paragraph that mentions Recover/Skip) with:

```markdown
## Unclean shutdown (GUI)

If Zarya was killed while a profile was running, the next startup **automatically** runs recovery actions and logs them in the UI:

- Restore system proxy (when Zarya-owned proxy restore on exit is enabled)
- Recover kill switch (when marker is present)
- Clean runtime temp config files (`config-xray.json`, `config-singbox.json`, `sing-box-tun.json`)

Zarya persists the **pre-enable system proxy snapshot** to `data/proxy-previous-state.json` when it first enables system proxy. After a crash, startup recovery can restore that snapshot or clear a Zarya-owned `127.0.0.1:<mixed-port>` proxy if no snapshot exists.

If networking is still broken after automatic recovery, follow the kill-switch / TUN sections below.

Recovery remains available even when experimental features are hidden on the stable release channel.
```

Do not rewrite the kill-switch / TUN sections below that heading.

- [ ] **Step 5: Commit (only if user asked to commit)**

```powershell
git add translations/zarya_en.ts translations/zarya_ru.ts docs/recovery.md
git commit -m "docs: document silent startup recovery; drop dialog i18n"
```

---

### Task 4: Verify

**Files:**
- None (verification only)

- [ ] **Step 1: Repo-wide leftover check**

Run:

```powershell
rg "StartupRecoveryDialog|Startup Recovery" src translations docs/recovery.md CMakeLists.txt
```

Expected:
- no `StartupRecoveryDialog`
- no modal title `Startup Recovery` in `src` / translations / `docs/recovery.md`
- `StartupRecovery` class references in `src/recovery` and `MainWindow` / `AppController` remain fine

- [ ] **Step 2: Rebuild**

```powershell
cmake --build build --config Release --target zarya
```

Expected: success.

- [ ] **Step 3: Manual smoke (from design spec)**

1. Stop Zarya. Create an empty file at the portable/runtime path used by the build, e.g. under the app data `runtime/config-xray.json` (or run once with a profile so the file exists, then kill the process).
2. Start Zarya → **no** Startup Recovery window.
3. Confirm the temp config was removed and the log shows unclean shutdown / cleanup lines.
4. Clean start with no leftovers → no recovery dialog and no spurious recovery error spam.

Optional (if easy): leave a kill-switch marker and confirm silent recover attempt (may need elevation as today).

---

## Spec coverage checklist

| Spec requirement | Task |
|------------------|------|
| Silent detect → apply on startup | Task 1 |
| Log detection + apply results | Task 1 |
| Remove modal / dialog class | Tasks 1–2 |
| CMake drop | Task 2 |
| Translation cleanup | Task 3 |
| `docs/recovery.md` update | Task 3 |
| No heuristic / settings changes | Global Constraints |
| Manual recovery paths untouched | Global Constraints |
| Success criteria / smoke | Task 4 |
