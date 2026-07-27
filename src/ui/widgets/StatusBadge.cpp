#include "ui/widgets/StatusBadge.h"

#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

namespace zarya {
namespace {

void badgeColors(StatusBadgeKind kind, const ThemeTokens& tokens, bool dark, QColor* bg,
                 QColor* fg)
{
    *fg = tokens.textPrimary;
    switch (kind) {
    case StatusBadgeKind::Ok:
        *bg = dark ? QColor(QStringLiteral("#238636")).darker(140) : QColor(QStringLiteral("#dafbe1"));
        *fg = tokens.success;
        break;
    case StatusBadgeKind::Warning:
        *bg = dark ? QColor(QStringLiteral("#9e6a03")).darker(150) : QColor(QStringLiteral("#fff8c5"));
        *fg = tokens.warning;
        break;
    case StatusBadgeKind::Error:
        *bg = dark ? QColor(QStringLiteral("#da3633")).darker(150) : QColor(QStringLiteral("#ffebe9"));
        *fg = tokens.danger;
        break;
    case StatusBadgeKind::Experimental:
        *bg = dark ? QColor(QStringLiteral("#8957e5")).darker(150) : QColor(QStringLiteral("#fbefff"));
        *fg = tokens.experimental;
        break;
    case StatusBadgeKind::Unsupported:
        *bg = tokens.panelBg;
        *fg = tokens.textSecondary;
        break;
    case StatusBadgeKind::Running:
        *bg = dark ? QColor(QStringLiteral("#1158c7")).darker(140) : QColor(QStringLiteral("#ddf4ff"));
        *fg = tokens.info;
        break;
    case StatusBadgeKind::Stopped:
        *bg = tokens.panelBg;
        *fg = tokens.textSecondary;
        break;
    case StatusBadgeKind::Neutral:
    default:
        *bg = tokens.panelBg;
        *fg = tokens.textSecondary;
        break;
    }
}

} // namespace

StatusBadge::StatusBadge(QWidget* parent)
    : QLabel(parent)
{
    setMargin(4);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        applyStyle(m_kind);
    });
    setKind(StatusBadgeKind::Neutral);
}

void StatusBadge::setKind(StatusBadgeKind kind)
{
    m_kind = kind;
    applyStyle(kind);
}

void StatusBadge::setBadgeText(const QString& text)
{
    setText(text);
}

void StatusBadge::applyStyle(StatusBadgeKind kind)
{
    const ThemeTokens tokens = ThemeManager::instance().tokens();
    QColor bg;
    QColor fg;
    badgeColors(kind, tokens, ThemeManager::instance().effectiveIsDark(), &bg, &fg);
    setStyleSheet(QStringLiteral("background:%1; color:%2; border-radius:%3px; padding:2px 6px;")
                      .arg(bg.name(QColor::HexRgb), fg.name(QColor::HexRgb))
                      .arg(tokens.radiusSm));
}

} // namespace zarya
