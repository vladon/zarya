# Zarya 0.26 Beta — TODO Audit

Audit date: Milestone 0.26 beta bugfix pass.

## Summary

| Classification | Count (approx.) |
|----------------|-----------------|
| Must fix before beta | 4 |
| Known limitation | 6 |
| Post-beta | 8 |
| Internal cleanup | 5 |

## Literal TODO markers

**Finding:** No literal `TODO`, `FIXME`, `HACK`, or `XXX` markers in `src/` or `src/ui/`.

Related user-visible or stub strings were classified below.

---

## Must fix before beta

| Item | Location | Notes |
|------|----------|-------|
| Choose existing Xray/sing-box binary | `src/ui/MainWindow.cpp` | Button resets to managed path instead of opening file picker |
| Startup proxy recovery after crash | `src/recovery/StartupRecovery.cpp` | Recovery controller had no persisted pre-Zarya state |
| First-run wizard without Xray | `src/ui/onboarding/FirstRunWizard.cpp` | Finish allowed before core install completes |
| Helper path policy vs external sing-box | `src/helperclient/HelperProcessManager.cpp` | TUN failed when sing-box lived outside default `cores/sing-box/` |

**0.26 status:** Proxy state persistence (`proxy-previous-state.json`) and configured sing-box parent for `--allowed-core-dir` address the recovery and helper items. Remaining Critical/High items should be verified on the regression checklist before release.

---

## Known limitation

| Item | Location | User impact |
|------|----------|-------------|
| **SingBoxAdapter stub** | `src/core/SingBoxAdapter.cpp` | `generateConfig()` returns "not implemented"; TUN uses `SingBoxConfigGenerator` instead — dead adapter path |
| **Rule-set update manager** | `src/runtime/singbox/SingBoxRuleSetManager.cpp:53` | Warning: "Rule-set update manager is not implemented yet." — manual `.srs` import only |
| **Basic diagnostics keeps hosts** | `DiagnosticsRedactor` / export UI | Basic mode redacts credentials but keeps hostnames and ports for support |
| **Unsigned builds** | `docs/release-packaging.md`, artifacts | No code signing or macOS notarization; Gatekeeper may warn |
| **No app self-update** | `docs/known-limitations.md` | Core Manager updates cores only |
| **macOS kill switch unsupported** | `src/platform/macos/` | No PF rules; recovery is instructions only |

---

## Post-beta

| Item | Area |
|------|------|
| sing-box rule-set auto-update manager | TUN / geo |
| App self-update channel | Packaging |
| Code signing and notarization | Packaging |
| Production service installer (Windows/macOS/Linux) | Platform |
| macOS kill switch (PF-based) | Security |
| Full localization beyond EN/RU | i18n |
| `SingBoxAdapter` unified with `SingBoxConfigGenerator` | Internal API |
| Kill switch "Allow traffic to selected proxy server" | Settings UI |

---

## Internal cleanup

| Item | Location |
|------|----------|
| `SingBoxAdapter::generateConfig` stub | `src/core/SingBoxAdapter.cpp` — delegate or remove to prevent misuse |
| `THIRD_PARTY_NOTICES.md` placeholder sections | Legal doc housekeeping |
| GeoData User-Agent version fallback | `src/geo/GeoDataManager.cpp` |
| Subscription User-Agent version fallback | `src/subscriptions/SubscriptionManager.cpp` |
| Stale milestone copy ("0.16" kill switch) | Settings / macOS kill switch strings |
| `runFirstRunWizard(bool force)` unused parameter | `src/ui/MainWindow.cpp` |

---

## Visible UI policy

- No user-visible strings containing literal `TODO`.
- Dead buttons must be hidden, disabled with explanation, or wired before beta.
- Experimental paths (TUN, helper, kill switch) stay behind warnings and default off.

## References

- [beta-bug-triage.md](beta-bug-triage.md) — severity definitions
- [beta-regression-checklist.md](beta-regression-checklist.md) — verification matrix
- [known-limitations.md](known-limitations.md) — user-facing limitations
