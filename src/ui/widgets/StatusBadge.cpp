#include "ui/widgets/StatusBadge.h"

namespace zarya {

StatusBadge::StatusBadge(QWidget* parent)
    : QLabel(parent)
{
    setMargin(4);
    setKind(StatusBadgeKind::Neutral);
}

void StatusBadge::setKind(StatusBadgeKind kind)
{
    applyStyle(kind);
}

void StatusBadge::setBadgeText(const QString& text)
{
    setText(text);
}

void StatusBadge::applyStyle(StatusBadgeKind kind)
{
    QString bg;
    QString fg = QStringLiteral("#1a1a1a");
    switch (kind) {
    case StatusBadgeKind::Ok:
        bg = QStringLiteral("#c8e6c9");
        break;
    case StatusBadgeKind::Warning:
        bg = QStringLiteral("#fff9c4");
        break;
    case StatusBadgeKind::Error:
        bg = QStringLiteral("#ffcdd2");
        fg = QStringLiteral("#b71c1c");
        break;
    case StatusBadgeKind::Experimental:
        bg = QStringLiteral("#e1bee7");
        break;
    case StatusBadgeKind::Unsupported:
        bg = QStringLiteral("#eeeeee");
        fg = QStringLiteral("#616161");
        break;
    case StatusBadgeKind::Running:
        bg = QStringLiteral("#b3e5fc");
        break;
    case StatusBadgeKind::Stopped:
        bg = QStringLiteral("#f5f5f5");
        break;
    case StatusBadgeKind::Neutral:
    default:
        bg = QStringLiteral("#eceff1");
        break;
    }
    setStyleSheet(QStringLiteral("background:%1; color:%2; border-radius:4px; padding:2px 6px;")
                      .arg(bg, fg));
}

} // namespace zarya
