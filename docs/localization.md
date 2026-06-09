# Localization

Zarya uses Qt's translation system. English is the **source language**; Russian is the first target locale.

## Adding user-facing strings

In `QObject` subclasses (dialogs, widgets, models with `Q_OBJECT`):

```cpp
label->setText(tr("Start"));
```

In non-`QObject` code (errors, helpers):

```cpp
#include "i18n/ZaryaTr.h"
ZaryaTr::tr("Xray is not installed");
```

For counts, use Qt plural forms:

```cpp
ZaryaTr::plural("%n profile(s) imported", count);
```

Enum and status labels are centralized in `TranslatableEnums` (`QCoreApplication::translate("Enums", ...)`).

## Do not translate

- Protocol IDs: VLESS, VMess, Trojan, Shadowsocks
- Core names: Xray, sing-box
- Error codes: `CORE_XRAY_MISSING`
- Config keys, JSON fields, URLs, file paths
- Raw Xray/sing-box/helper stdout/stderr

Translate display labels and user explanations only.

## Translation files

| File | Purpose |
|------|---------|
| `translations/zarya_en.ts` | Source catalog (English) |
| `translations/zarya_ru.ts` | Russian translations |
| `translations/zarya_*.qm` | Compiled catalogs loaded at runtime |

Runtime path: `{applicationDir}/translations/zarya_ru.qm`

## Updating translation sources

After adding or changing `tr()` strings:

```bash
cmake --build build --target zarya_lupdate
```

Apply Russian entries (project helper):

```bash
python scripts/fill-ru-translations.py
```

Edit `translations/zarya_ru.ts` in Qt Linguist for fine-tuning.

## Compiling translations

```bash
cmake --build build --target zarya_lrelease
```

Release builds copy `.qm` files next to the executable automatically.

## Language settings

Settings → General → Language:

- **System default** — Russian on `ru_*` locales, English otherwise
- **English**
- **Русский**

Changing language shows a restart notice; a full restart applies translations everywhere.

## Checks

```bash
python scripts/check-translations.py
```

Verifies TS files exist, flags Russian text in C++ source strings, and reports Russian coverage.

## For translators

- Keep `%1`, `%2`, `%n` placeholders and order intact
- Preserve `&` accelerator markers where possible
- File dialog filters must keep extensions: `*.zarya-backup.zip`
- Russian plural forms use `<numerusform>` entries in `.ts` files
