# Vendored Desktop App dependencies

The source dependencies in this directory are tracked as ordinary repository
files. Zarya does not use Git submodules, so a normal clone contains everything
needed from these upstream projects.

## Pinned revisions

| Path | Upstream | Revision |
|------|----------|----------|
| `GSL` | <https://github.com/Microsoft/GSL.git> | `a75212b9f3b14162edd62d540cbf9273d5a59d20` |
| `cmake_helpers` | <https://github.com/desktop-app/cmake_helpers.git> | `80cd031dc4c81805b8bb118e8250356afaad6614` |
| `cmake_helpers/external/glib/cppgir` | <https://gitlab.com/mnauw/cppgir.git> | `47cf94f83b54cda59018135601e19d7fb0c77776` |
| `cmake_helpers/external/glib/cppgir/expected-lite` | <https://github.com/martinmoene/expected-lite.git> | `95b9cb015fa17baa749c2b396b335906e1596a9e` |
| `codegen` | <https://github.com/desktop-app/codegen.git> | `1996c7c61e62eeb95e0fd35dc806238a1760eea7` |
| `expected` | <https://github.com/TartanLlama/expected.git> | `1770e3559f2f6ea4a5fb4f577ad22aeb30fbd8e4` |
| `kcoreaddons` | <https://github.com/KDE/kcoreaddons.git> | `f34c4508e6c1ed417fdd672f8a916341392eafc6` |
| `lib_base` | <https://github.com/desktop-app/lib_base.git> | `82d182a275e197fd717fecc86193d9d91f4fc5b5` |
| `lib_crl` | <https://github.com/desktop-app/lib_crl.git> | `7a165302fed408c84b2d1c2513e35a21a141da44` |
| `lib_rpl` | <https://github.com/desktop-app/lib_rpl.git> | `c57cccffb01d85570decd7fccb88419c9a682e63` |
| `lib_ui` | <https://github.com/desktop-app/lib_ui.git> | `640bc4c8c6a5b2a40986a86167c48152fc35e8ce` |
| `lz4` | <https://github.com/lz4/lz4.git> | `0774d05537f9762f838f7ab541b7765f1a729cb5` |
| `range-v3` | <https://github.com/ericniebler/range-v3.git> | `108f93c279c8f9cec175dac361084983d0176e99` |
| `range-v3/doc/gh-pages` | <https://github.com/ericniebler/range-v3.git> | `2dae74bb693e42d850fb0adcc9045c5b71fbdeae` |
| `xxHash` | <https://github.com/Cyan4973/xxHash.git> | `16512d84ae6fc0836dbbe9f5732d3f661175ebab` |

## Updating a dependency

1. Check out the desired upstream revision in a temporary directory.
2. Replace only that dependency's files here. Do not copy `.git` metadata or
   upstream `.gitmodules`; materialize any nested dependency in the same way.
3. Preserve all upstream license and copyright files.
4. Update the revision table above and `THIRD_PARTY_NOTICES.md`.
5. Configure and test Zarya on the affected platforms.

Local compatibility patches belong in Zarya's CMake build tree where possible,
so the vendored upstream sources remain easy to compare with their pinned
revisions.
