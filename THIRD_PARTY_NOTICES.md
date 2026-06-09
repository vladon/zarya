# Third-Party Notices

Zarya bundles or links the following third-party components.

## Qt 6

Zarya is built with [Qt 6](https://www.qt.io/) (modules including Core, Gui, Widgets, Network).

Qt is licensed under the GNU Lesser General Public License (LGPL) version 3 and other licenses depending on the module. See the Qt documentation and your Qt installation for the exact license texts.

When distributing shared-Qt builds, comply with Qt LGPL obligations (including providing a written offer to supply Qt source code where required).

## Platform libraries (Windows helper)

On Windows, `zarya-helper` may link system libraries such as `Fwpuclnt`, `Ws2_32`, and `iphlpapi` for the experimental WFP kill switch proof of concept.

## Icons and assets

Application icons and bundled imagery are part of the Zarya project unless otherwise noted in the repository.

## Not bundled by default

**Xray** and **sing-box** are **not** included in the default Zarya release artifact.

Zarya can download them from upstream release sources via **Core Manager**. Use upstream project names and license terms when installing cores:

- [Xray](https://github.com/XTLS/Xray-core)
- [sing-box](https://github.com/SagerNet/sing-box)

Geo data files (`geoip.dat`, `geosite.dat`) and sing-box rule sets are also downloaded separately when needed.

## TODO

Automated generation of this file from CMake linked targets is not implemented yet. Update manually when adding bundled dependencies.
