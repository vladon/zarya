#include "ui/desktopapp/StatusBadgeLibUiEmbed.h"

#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/labels.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

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

StatusBadgeLibUiEmbed::StatusBadgeLibUiEmbed(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, QString(), st::defaultFlatLabel);
    m_labelHost = label;

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        applyColors();
        update();
    });
    applyColors();
}

void StatusBadgeLibUiEmbed::setKind(StatusBadgeKind kind)
{
    m_kind = kind;
    applyColors();
    update();
}

void StatusBadgeLibUiEmbed::setBadgeText(const QString& text)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_labelHost);
    label->setText(text);
    const int padX = 6;
    const int padY = 2;
    const int textW = std::max(label->textMaxWidth(), 1);
    label->resizeToWidth(textW);
    const int textH = std::max(label->height(), 1);
    setFixedSize(textW + padX * 2, textH + padY * 2);
    layoutLabel();
}

void StatusBadgeLibUiEmbed::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const ThemeTokens tokens = ThemeManager::instance().tokens();
    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), tokens.radiusSm,
                        tokens.radiusSm);
    p.fillPath(path, m_bg);
}

void StatusBadgeLibUiEmbed::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutLabel();
}

void StatusBadgeLibUiEmbed::applyColors()
{
    badgeColors(m_kind, ThemeManager::instance().tokens(),
                ThemeManager::instance().effectiveIsDark(), &m_bg, &m_fg);
    auto* label = static_cast<Ui::FlatLabel*>(m_labelHost);
    label->setTextColorOverride(m_fg);
}

void StatusBadgeLibUiEmbed::layoutLabel()
{
    if (!m_labelHost) {
        return;
    }
    const int padX = 6;
    const int padY = 2;
    m_labelHost->move(padX, padY);
}

} // namespace zarya
