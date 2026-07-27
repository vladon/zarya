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
    t.accent = QColor(QStringLiteral("#1f6feb"));
    t.accentHover = QColor(QStringLiteral("#1858c7"));
    t.accentFg = QColor(QStringLiteral("#ffffff"));
    t.border = QColor(QStringLiteral("#d0d7de"));
    t.danger = QColor(QStringLiteral("#cf222e"));
    t.warning = QColor(QStringLiteral("#bf8700"));
    t.success = QColor(QStringLiteral("#1a7f37"));
    t.info = QColor(QStringLiteral("#0969da"));
    t.experimental = QColor(QStringLiteral("#8250df"));
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
    t.accent = QColor(QStringLiteral("#2f81f7"));
    t.accentHover = QColor(QStringLiteral("#58a6ff"));
    t.accentFg = QColor(QStringLiteral("#ffffff"));
    t.border = QColor(QStringLiteral("#30363d"));
    t.danger = QColor(QStringLiteral("#f85149"));
    t.warning = QColor(QStringLiteral("#d29922"));
    t.success = QColor(QStringLiteral("#3fb950"));
    t.info = QColor(QStringLiteral("#58a6ff"));
    t.experimental = QColor(QStringLiteral("#a371f7"));
    t.radiusSm = 4;
    t.radiusMd = 6;
    return t;
}

} // namespace zarya
