# Third-Party Notices

Zarya bundles or links the following third-party components.

Zarya itself is dual-licensed MIT | GPLv3+ for authored source (see `LICENSE`).
Official binaries always link Desktop App Toolkit and must be distributed under
GPLv3+.

## Qt 6

Zarya is built with [Qt 6](https://www.qt.io/) (modules including Core, Gui, Widgets, Network).

Qt is licensed under the GNU Lesser General Public License (LGPL) version 3 and other licenses depending on the module. See the Qt documentation and your Qt installation for the exact license texts.

When distributing shared-Qt builds, comply with Qt LGPL obligations (including providing a written offer to supply Qt source code where required).

## Desktop App Toolkit (`lib_ui`)

Zarya links libraries from the [Desktop App Toolkit](https://github.com/desktop-app), including:

- [lib_ui](https://github.com/desktop-app/lib_ui)
- [lib_base](https://github.com/desktop-app/lib_base)
- [lib_rpl](https://github.com/desktop-app/lib_rpl)
- [lib_crl](https://github.com/desktop-app/lib_crl)
- [codegen](https://github.com/desktop-app/codegen) / [cmake_helpers](https://github.com/desktop-app/cmake_helpers)

These are licensed under **GPLv3+** with an OpenSSL linking exception. See
[desktop-app/legal](https://github.com/desktop-app/legal/blob/master/LEGAL).
A Zarya binary that includes them is a GPLv3+ derivative for distribution purposes.

## Platform libraries (Windows helper)

On Windows, `zarya-helper` may link system libraries such as `Fwpuclnt`, `Ws2_32`, and `iphlpapi` for the experimental WFP kill switch proof of concept.

## Icons and assets

Application icons and bundled imagery are part of the Zarya project unless otherwise noted in the repository.

## Not bundled by default

**Xray** and **sing-box** are **not** included in the default Zarya release artifact.

Zarya can download them from upstream release sources via **Core Manager**. Use upstream project names and license terms when installing cores:

- [Xray](https://github.com/XTLS/Xray-core)
- [sing-box](https://github.com/SagerNet/sing-box)

Geo data files (`geoip.dat`, `geosite.dat`) ship as a pinned **runetfreedom** seed in release packages (next to Xray) and can also be updated via **Geo Data Manager**. sing-box rule sets are downloaded separately via rule-set tools. Built-in geo sources include:

- [Loyalsoldier/v2ray-rules-dat](https://github.com/Loyalsoldier/v2ray-rules-dat)
- [runetfreedom/russia-v2ray-rules-dat](https://github.com/runetfreedom/russia-v2ray-rules-dat)
- [Chocolate4U/Iran-v2ray-rules](https://github.com/Chocolate4U/Iran-v2ray-rules)

Use upstream project names and license terms when downloading those files.

## TODO

Automated generation of this file from CMake linked targets is not implemented yet. Update manually when adding bundled dependencies.
