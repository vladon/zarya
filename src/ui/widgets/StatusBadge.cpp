#include "ui/widgets/StatusBadge.h"

#include "ui/desktopapp/StatusBadgeLibUiEmbed.h"

#include <QVBoxLayout>

namespace zarya {

StatusBadge::StatusBadge(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_embed = new StatusBadgeLibUiEmbed(this);
    layout->addWidget(m_embed);
    setKind(StatusBadgeKind::Neutral);
}

void StatusBadge::setKind(StatusBadgeKind kind)
{
    m_kind = kind;
    m_embed->setKind(kind);
}

void StatusBadge::setBadgeText(const QString& text)
{
    m_embed->setBadgeText(text);
}

} // namespace zarya
