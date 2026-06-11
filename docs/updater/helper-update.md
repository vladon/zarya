# Helper update

`zarya-helper` is privileged. Updating it is more sensitive than updating the GUI.

## Rules

- Helper update requires signed artifact
- Helper must be stopped safely
- Service must be restarted
- Rollback must be possible
- Helper version compatibility with GUI must be checked

## Separation

GUI update ≠ helper update. Do not update helper in the same step as app self-update without a dedicated verified flow.

See also [installer-security.md](../installer/installer-security.md).
