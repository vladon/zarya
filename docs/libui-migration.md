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
5. **Management surfaces**
   - Migrate Core, Routing, DNS, Geo Data, Rule Set, backup, and diagnostics workflows using the
     shared interaction layer.
6. **Experimental and update surfaces**
   - Migrate gated sing-box TUN, helper, kill-switch, and updater UI without changing
     `FeatureGate` behavior.
7. **Convergence**
   - Remove application QSS for controls that no longer exist.
   - Delete obsolete Qt-only wrappers and verify that remaining Qt widgets match the boundary
     above.

Each numbered stage may span multiple focused PRs. A PR migrates one coherent user workflow and
must not mix unrelated backend changes.

## Acceptance for every migrated surface

- Behavior, persistence, feature gating, and recovery actions remain unchanged.
- English and Russian strings are complete and fit at common display scaling.
- Tab order, keyboard activation, focus visibility, accessible names, and screen-reader roles
  are verified.
- Light, dark, and system themes update both Qt hosts and `lib_ui` content without restart unless
  the existing setting explicitly requires one.
- Semantic state always includes text; color is supplementary.
- Animations are state-driven, short, and safe when reduced motion is requested.
- Linux, macOS, and static-Qt Windows builds pass; the migrated workflow receives a targeted
  smoke or unit test where feasible.
