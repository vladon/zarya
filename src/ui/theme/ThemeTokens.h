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
    QColor accentFill;
    QColor accentFillHover;
    QColor accentFg;
    QColor border;
    QColor danger;
    QColor warning;
    QColor success;
    QColor info;
    QColor experimental;
    QColor dangerSurface;
    QColor warningSurface;
    QColor successSurface;
    QColor infoSurface;
    QColor experimentalSurface;
    int radiusSm = 4;
    int radiusMd = 6;
};

ThemeTokens lightTokens();
ThemeTokens darkTokens();

} // namespace zarya
