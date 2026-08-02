# Desktop App Toolkit UI migration

## Target

Move all application-owned visual surfaces to Desktop App Toolkit (`lib_ui`) while preserving
Zarya's calm, state-first product identity. Qt remains the platform and application framework;
the migration does not replace Qt itself.

`PRODUCT.md` defines product intent and `DESIGN.md` defines the visual contract. New or migrated
surfaces must follow both.

## Boundary

Use `lib_ui` for application-owned labels, buttons, fields, checkboxes, selectors, scroll
containers, status feedback, forms, panels, overlays, and dialogs.

Keep Qt where it is infrastructure or the safer native primitive:

- `QApplication`, window lifecycle, object ownership, signals, models, networking, and storage.
- `QSystemTrayIcon` and unavoidable platform-native menu integration.
- Native file/folder pickers, clipboard, desktop services, and accessibility bridges.
- Model/view tables until a `lib_ui` replacement supports Zarya's sorting, selection,
  keyboard navigation, accessibility, and large datasets without regression.

Qt host widgets may remain temporarily around migrated content. A host is not considered an
unmigrated surface when it owns no visual styling or interaction.

The boundary is enforced by `scripts/check-libui-boundary.py` across the complete `src/` tree.
Its allowlist records the exact legacy visual-control references by file and control type,
including dialogs owned outside `src/ui`. New references and silent growth fail the check, and CI
rejects allowlist increases against the PR base. Removing a migrated control also fails until the
reviewed inventory is regenerated:

```sh
python3 scripts/check-libui-boundary.py --print-current
```

Replace `scripts/libui-qt-widget-allowlist.json` with that output only when the PR demonstrably
reduces the inventory. Approved infrastructure and native/model-view boundaries are not counted.

## Delivery sequence

1. **Foundation and status**
   - Bridge Zarya theme tokens into the Desktop App Toolkit palette.
   - Migrate configured and first-run status surfaces.
   - Remove the developer-only toolkit spike.
2. **Shared interaction layer**
   - Introduce Zarya-owned wrappers for primary/secondary/destructive buttons, fields, status
     chips, form rows, dialog actions, and asynchronous feedback.
   - Replace `QMessageBox` use through a central `lib_ui` presenter, retaining a native fallback
     only for startup failures before toolkit initialization.
   - Progress: the shared button roles and synchronous message presenter are available, including
     long-message scrolling, keyboard handling, semantic text, and the pre-toolkit native
     fallback. Share-link import, profile deletion, kill-switch recovery, and update prompts use
     the toolkit presenter and its multi-action model.
3. **Main application shell**
   - Migrate profile actions, filters, empty state, log controls, and operational feedback.
   - Keep `QTableView` and the log text engine behind toolkit-styled hosts until their replacement
     meets the boundary requirements.
   - Progress: profile actions, profile and log filter selectors, the profile-list empty state,
     log toolbar actions, localized runtime status feedback, and single- and multi-action
     operational feedback use `lib_ui`; the primary action strip exposes its profile selector as
     a labelled menu button and its overflow action with menu semantics. The log text engine
     remains a Qt primitive under the documented boundary.
4. **Primary workflows**
   - Migrate first-run setup, link import, profile editing, subscriptions, and Settings.
   - Put advanced settings behind explicit progressive disclosure.
   - Progress: profile editing uses shared toolkit text/number fields, selectors, checkbox,
     labeled rows, inline validation, dialog actions, pill tabs, and scrolling while preserving
     the existing four-section profile model. Share-link import and the first-run parse/checklist
     surfaces use shared toolkit multiline and body-text controls. The first-run flow uses a
     host-only `QDialog`, stacked pages, toolkit navigation, content, selectors, fields, checks,
     actions, and validation feedback. Subscription editing and management use
     the shared toolkit fields, checkbox, multiline input, action buttons, empty/update states,
     and message presenter; the subscription table remains a Qt model/view boundary. General,
     desktop-behavior, and startup settings use toolkit sections, selectors, checks, text, and
     number fields while retaining the existing settings persistence and autostart backend.
     Stable core, system-proxy, and testing settings also use toolkit fields, selectors, actions,
     checks, status text, and sections; native executable picking remains a platform boundary.
     Active Routing and DNS selection and their management actions use toolkit sections and
     selectors; the dedicated manager workflows remain in the management migration stage. App
     updates, core updates, release-channel selection, and the stable experimental-feature gate
     use toolkit selectors, fields, checks, actions, text, and number controls without changing
     `FeatureGate` policy.
5. **Management surfaces**
   - Migrate Core, Routing, DNS, Geo Data, Rule Set, backup, and diagnostics workflows using the
     shared interaction layer.
   - Progress: Routing Manager actions, empty state, confirmations, and feedback use the shared
     toolkit layer; its table remains a Qt model/view boundary. Routing profile and rule editing
     use toolkit fields, selectors, sections, actions, inline validation, and empty states; the
     rules table remains a Qt model/view boundary. Routing JSON preview uses a read-only toolkit
     text area and action.
   - DNS Manager actions, empty state, validation confirmations, and feedback use the toolkit
     layer; its profile table remains a Qt model/view boundary.
   - DNS profile and server editors use toolkit tabs, forms, selectors, fields, checks, actions,
     empty states, and feedback; the DNS server table remains a Qt model/view boundary.
   - Core Manager actions, status text, and warnings use the toolkit layer; the core inventory
     table and bounded log text engine remain Qt boundaries.
   - Geo Data Manager source selection, options, actions, status text, and warnings use the
     toolkit layer; the file inventory table, bounded log text engine, and native folder opening
     remain Qt/platform boundaries.
   - Rule Set Manager status text, sections, empty states, actions, and feedback use the toolkit
     layer; rule-set tables, bounded log text, and native file/folder pickers remain boundaries.
   - Backup export/import types, category options, mode selectors, paths, summary text, warnings,
     confirmations, and actions use the toolkit layer; the import preview table and native file
     pickers remain Qt/platform boundaries.
   - Diagnostics redaction and include options, output path, preview context, multi-action result
     feedback, and actions use the toolkit layer; the preview file list and native save/folder
     integration remain Qt/platform boundaries.
6. **Experimental and update surfaces**
   - Migrate gated sing-box TUN, helper, kill-switch, and updater UI without changing
     `FeatureGate` behavior.
   - Progress: readiness/recovery guidance and its navigation actions use the toolkit layer
     without changing availability checks or recovery routing.
   - Safe-exit recovery options and actions use the toolkit layer without changing runtime,
     system-proxy, or kill-switch cleanup behavior.
   - sing-box config preview content, warnings, actions, validation results, and save failures use
     the toolkit layer; the native save picker remains a platform boundary.
   - App updater metadata, progress/status text, plan details, actions, verification/staging
     errors, install confirmation, and result feedback use the toolkit layer; update planning,
     verification, platform installers, and native file/folder integration remain unchanged.
     Its complete user-facing string set is synchronized in EN/RU catalogs.
   - Stable/beta/experimental banner text and dismissal use the toolkit layer, with a
     theme-aware painted warning surface and unchanged release-channel policy.
   - Settings, including gated TUN, helper, kill-switch, update, routing, DNS, startup, and
     desktop behavior controls, use shared toolkit form primitives and feedback while preserving
     the existing `FeatureGate`, persistence, native file pickers, and recovery behavior.
   - Controller/runtime start warnings, geo/DNS/rule-set choices, TUN privilege and kill-switch
     recovery prompts, updater startup notices, and structured application errors use the shared
     toolkit presenter without changing backend behavior.
7. **Convergence**
   - Complete: application QSS selectors for controls that no longer exist and obsolete Qt-only
     includes are removed. Settings scrolling and the diagnostics preview use toolkit controls.
     Startup recovery progress and the ambiguous-profile selector also use toolkit content rather
     than native `QProgressDialog` or `QInputDialog` surfaces.
   - The final full-source allowlist contains only the centralized native `QMessageBox` fallback
     used before Desktop App Toolkit initialization. The persisted profile/log split uses a
     keyboard-accessible toolkit handle while retaining the approved Qt table and log engines.
     Model/view tables, log text engines, tray/native menus, file dialogs, and infrastructure Qt
     remain approved boundaries. The two exact native-message keyboard handlers are excluded from
     the inventory; all other application source directories are scanned.

Each numbered stage may span multiple focused PRs. A PR migrates one coherent user workflow and
must not mix unrelated backend changes.

During the active migration, each focused PR is built and smoke-tested locally on macOS before
merge. Cross-platform Windows and Linux verification is performed in dedicated hardening passes
and remains required before a release, rather than blocking every migration PR.

## Acceptance for every migrated surface

- Behavior, persistence, feature gating, and recovery actions remain unchanged.
- English and Russian strings are complete and fit at common display scaling.
- Tab order, keyboard activation, focus visibility, accessible names, and screen-reader roles
  are verified. Shared form controls have an automated headless accessibility contract covering
  roles, labelled fields and selectors, focus proxies, and assistive activation.
- Light, dark, and system themes update both Qt hosts and `lib_ui` content without restart unless
  the existing setting explicitly requires one.
- Semantic state always includes text; color is supplementary.
- Animations are state-driven and short. The global toolkit animation state follows Qt desktop
  effects and zero-duration style hints, including theme, style, and application-state changes at
  runtime, so reduced motion requests take effect across migrated surfaces.
- The local macOS build and relevant smoke or unit tests pass for each migration PR.
- Linux and static-Qt Windows builds pass in the cross-platform hardening pass before release.
