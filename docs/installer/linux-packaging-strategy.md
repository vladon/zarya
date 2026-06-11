# Linux Packaging Strategy

## Distribution options

### AppImage

Pros:

- easy cross-distro testing
- no root install
- good beta format

Cons:

- system service/polkit integration not automatic

### deb

Pros:

- native Ubuntu/Debian install
- can install systemd/polkit files

Cons:

- Debian policy details
- root package install

### rpm

Pros:

- Fedora/RHEL support

Cons:

- separate packaging work

## Recommendation

- **beta:** tarball/AppImage
- **production Linux:** deb first, rpm later

## 0.31 skeleton

- `packaging/linux/appimage/` — AppDir, AppRun, desktop file
- `packaging/linux/deb/` — DEBIAN control templates
- `packaging/linux/rpm/` — spec template

Scripts are placeholders unless already working.
