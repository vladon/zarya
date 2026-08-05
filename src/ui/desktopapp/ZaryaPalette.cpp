#include "ui/desktopapp/ZaryaPalette.h"

#include "ui/theme/ThemeTokens.h"

#include "ui/style/style_core.h"
#include "ui/style/style_core_palette.h"

#include <QColor>
#include <QLatin1String>

namespace zarya {
namespace {

void setColor(const char* name, const QColor& color)
{
    style::main_palette::setColor(
        QLatin1String(name),
        static_cast<uchar>(color.red()),
        static_cast<uchar>(color.green()),
        static_cast<uchar>(color.blue()),
        static_cast<uchar>(color.alpha()));
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

} // namespace

void applyDesktopAppPalette(const ThemeTokens& tokens)
{
    setColor("windowBg", tokens.surfaceBg);
    setColor("windowFg", tokens.textPrimary);
    setColor("windowBgOver", tokens.panelBg);
    setColor("windowBgRipple", tokens.border);
    setColor("windowFgOver", tokens.textPrimary);
    setColor("windowSubTextFg", tokens.textSecondary);
    setColor("windowSubTextFgOver", tokens.textPrimary);
    setColor("windowBoldFg", tokens.textPrimary);
    setColor("windowBoldFgOver", tokens.textPrimary);
    setColor("windowBgActive", tokens.accentFill);
    setColor("windowFgActive", tokens.accentFg);
    setColor("windowActiveTextFg", tokens.accent);
    setColor("windowShadowFgFallback", tokens.border);

    setColor("activeButtonBg", tokens.accentFill);
    setColor("activeButtonBgOver", tokens.accentFillHover);
    setColor("activeButtonBgRipple", withAlpha(tokens.accentFg, 48));
    setColor("activeButtonFg", tokens.accentFg);
    setColor("activeButtonFgOver", tokens.accentFg);
    setColor("lightButtonBg", tokens.surfaceBg);
    setColor("lightButtonBgOver", tokens.panelBg);
    setColor("lightButtonBgRipple", tokens.border);
    setColor("lightButtonFg", tokens.accent);
    setColor("lightButtonFgOver", tokens.accentHover);

    setColor("inputBorderFg", tokens.border);
    setColor("placeholderFg", tokens.textSecondary);
    setColor("placeholderFgActive", tokens.textSecondary);
    setColor("checkboxFg", tokens.border);
    setColor("menuBg", tokens.surfaceBg);
    setColor("menuBgOver", tokens.panelBg);
    setColor("menuBgRipple", tokens.border);
    setColor("menuFgDisabled", tokens.textDisabled);
    setColor("menuSeparatorFg", tokens.border);
    setColor("boxBg", tokens.surfaceBg);
    setColor("boxTextFg", tokens.textPrimary);
    setColor("boxTitleFg", tokens.textPrimary);
    setColor("boxTextFgGood", tokens.success);
    setColor("boxTextFgError", tokens.danger);
    setColor("tooltipBg", tokens.surfaceBg);
    setColor("tooltipFg", tokens.textPrimary);
    setColor("tooltipBorderFg", tokens.border);

    style::NotifyPaletteChanged();
}

} // namespace zarya
