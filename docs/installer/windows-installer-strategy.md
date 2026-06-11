# Windows Installer Strategy

## Candidate technologies

### WiX Toolset

Pros:

- MSI-native
- good service installation support
- enterprise-friendly
- repair/uninstall semantics

Cons:

- more complex authoring

### NSIS

Pros:

- simpler
- common for desktop apps
- easy custom UI

Cons:

- less MSI-native
- service handling is custom

### Inno Setup

Pros:

- simple
- good desktop installer UX

Cons:

- service/helper flows require custom scripts

## Recommendation

Use **WiX** for production installer planning. Keep NSIS/Inno as fallback if speed matters.

## 0.31 scope

- WiX skeleton under `packaging/windows/wix/`
- **Do not ship production MSI yet**
- WiX is not required for normal builds

Skeleton components:

- `ZaryaAppComponents` — app binaries, translations, docs
- `ZaryaHelperServiceComponents` — optional service (future)
