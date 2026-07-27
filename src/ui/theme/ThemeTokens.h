#pragma once

#include <QColor>

namespace zarya {

struct ThemeTokens {
    QColor windowBg;
    QColor surfaceBg;
    QColor panelBg;
    QColor textPrimary;
    QColor textSecondary;
    QColor textDisabled;
    QColor accent;
    QColor accentHover;
    QColor accentFg;
    QColor border;
    QColor danger;
    QColor warning;
    QColor success;
    QColor info;
    QColor experimental;
    int radiusSm = 4;
    int radiusMd = 6;
};

ThemeTokens lightTokens();
ThemeTokens darkTokens();

} // namespace zarya
