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

The boundary is enforced by `scripts/check-libui-boundary.py`. Its allowlist records the exact
legacy visual-control references by file and control type. New references and silent growth fail
the check, and CI rejects allowlist increases against the PR base. Removing a migrated control
also fails until the reviewed inventory is regenerated:

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
     fallback. Share-link import, profile deletion, and kill-switch startup recovery use the
     toolkit presenter and its multi-action model; update prompts remain on the Qt presenter.
3. **Main application shell**
   - Migrate profile actions, filters, empty state, log controls, and operational feedback.
   - Keep `QTableView` and the log text engine behind toolkit-styled hosts until their replacement
     meets the boundary requirements.
   - Progress: profile actions, profile and log filter selectors, the profile-list empty state,
     log toolbar actions, and single- and multi-action operational feedback use `lib_ui`; the log
     text engine remains a Qt primitive under the documented boundary.
4. **Primary workflows**
   - Migrate first-run setup, link import, profile editing, subscriptions, and Settings.
   - Put advanced settings behind explicit progressive disclosure.
   - Progress: profile editing uses shared toolkit text/number fields, selectors, checkbox,
     labeled rows, inline validation, dialog actions, pill tabs, and scrolling while preserving
     the existing four-section profile model. Share-link import and the first-run parse/checklist
     surfaces use shared toolkit multiline and body-text controls. The first-run wizard keeps
     `QWizard` only as its temporary host; its content, selectors, fields, checks, actions, and
     validation feedback use the shared toolkit layer. Subscription editing and management use
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
   - App updater metadata, status, details, actions, confirmations, verification errors, staging
     failures, and restart feedback use the toolkit layer; native manifest picking and folder
     opening remain platform boundaries.
7. **Convergence**
   - Remove application QSS for controls that no longer exist.
   - Delete obsolete Qt-only wrappers and verify that remaining Qt widgets match the boundary
     above.

Each numbered stage may span multiple focused PRs. A PR migrates one coherent user workflow and
must not mix unrelated backend changes.

During the active migration, each focused PR is built and smoke-tested locally on macOS before
merge. Cross-platform Windows and Linux verification is performed in dedicated hardening passes
and remains required before a release, rather than blocking every migration PR.

## Acceptance for every migrated surface

- Behavior, persistence, feature gating, and recovery actions remain unchanged.
- English and Russian strings are complete and fit at common display scaling.
- Tab order, keyboard activation, focus visibility, accessible names, and screen-reader roles
  are verified.
- Light, dark, and system themes update both Qt hosts and `lib_ui` content without restart unless
  the existing setting explicitly requires one.
- Semantic state always includes text; color is supplementary.
- Animations are state-driven, short, and safe when reduced motion is requested.
- The local macOS build and relevant smoke or unit tests pass for each migration PR.
- Linux and static-Qt Windows builds pass in the cross-platform hardening pass before release.
