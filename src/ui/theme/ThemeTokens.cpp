#include "ui/theme/ThemeTokens.h"

namespace zarya {

ThemeTokens lightTokens()
{
    ThemeTokens t;
    t.windowBg = QColor(QStringLiteral("#f4f6f8"));
    t.surfaceBg = QColor(QStringLiteral("#ffffff"));
    t.panelBg = QColor(QStringLiteral("#eef1f4"));
    t.textPrimary = QColor(QStringLiteral("#1b1f24"));
    t.textSecondary = QColor(QStringLiteral("#5b6570"));
    t.textDisabled = QColor(QStringLiteral("#9aa3ad"));
    t.accent = QColor(QStringLiteral("#0969da"));
    t.accentHover = QColor(QStringLiteral("#0550ae"));
    t.accentFill = QColor(QStringLiteral("#1f6feb"));
    t.accentFillHover = QColor(QStringLiteral("#1858c7"));
    t.accentFg = QColor(QStringLiteral("#ffffff"));
    t.border = QColor(QStringLiteral("#d0d7de"));
    t.danger = QColor(QStringLiteral("#cf222e"));
    t.warning = QColor(QStringLiteral("#946200"));
    t.success = QColor(QStringLiteral("#1a7f37"));
    t.info = QColor(QStringLiteral("#0969da"));
    t.experimental = QColor(QStringLiteral("#8250df"));
    t.dangerSurface = QColor(QStringLiteral("#ffebe9"));
    t.warningSurface = QColor(QStringLiteral("#fff8c5"));
    t.successSurface = QColor(QStringLiteral("#dafbe1"));
    t.infoSurface = QColor(QStringLiteral("#ddf4ff"));
    t.experimentalSurface = QColor(QStringLiteral("#fbefff"));
    t.radiusSm = 4;
    t.radiusMd = 6;
    return t;
}

ThemeTokens darkTokens()
{
    ThemeTokens t;
    t.windowBg = QColor(QStringLiteral("#0d1117"));
    t.surfaceBg = QColor(QStringLiteral("#161b22"));
    t.panelBg = QColor(QStringLiteral("#21262d"));
    t.textPrimary = QColor(QStringLiteral("#e6edf3"));
    t.textSecondary = QColor(QStringLiteral("#8b949e"));
    t.textDisabled = QColor(QStringLiteral("#6e7681"));
    t.accent = QColor(QStringLiteral("#58a6ff"));
    t.accentHover = QColor(QStringLiteral("#79c0ff"));
    t.accentFill = QColor(QStringLiteral("#1f6feb"));
    t.accentFillHover = QColor(QStringLiteral("#1858c7"));
    t.accentFg = QColor(QStringLiteral("#ffffff"));
    t.border = QColor(QStringLiteral("#30363d"));
    t.danger = QColor(QStringLiteral("#f85149"));
    t.warning = QColor(QStringLiteral("#d29922"));
    t.success = QColor(QStringLiteral("#3fb950"));
    t.info = QColor(QStringLiteral("#58a6ff"));
    t.experimental = QColor(QStringLiteral("#a371f7"));
    t.dangerSurface = QColor(QStringLiteral("#2c1c1d"));
    t.warningSurface = QColor(QStringLiteral("#3d2e00"));
    t.successSurface = QColor(QStringLiteral("#12261e"));
    t.infoSurface = QColor(QStringLiteral("#121d2f"));
    t.experimentalSurface = QColor(QStringLiteral("#211a2c"));
    t.radiusSm = 4;
    t.radiusMd = 6;
    return t;
}

} // namespace zarya
