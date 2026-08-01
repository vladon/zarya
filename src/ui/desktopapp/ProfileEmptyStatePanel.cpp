#include "ui/desktopapp/ProfileEmptyStatePanel.h"

#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/labels.h"

#include <QResizeEvent>

namespace zarya {
namespace {

const style::FlatLabel& emptyStateStyle()
{
    static const auto result = [] {
        auto style = st::defaultSubTextLabel;
        style.align = style::al_center;
        style.margin = QMargins(12, 12, 12, 12);
        return style;
    }();
    return result;
}

} // namespace

ProfileEmptyStatePanel::ProfileEmptyStatePanel(QWidget* parent)
    : QWidget(parent)
{
    auto* label = Ui::CreateChild<Ui::FlatLabel>(this, QString(), emptyStateStyle());
    m_label = label;
}

void ProfileEmptyStatePanel::setMessage(const QString& message)
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    label->setText(message);
    label->setAccessibleName(message);
    relayout();
}

void ProfileEmptyStatePanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void ProfileEmptyStatePanel::relayout()
{
    auto* label = static_cast<Ui::FlatLabel*>(m_label);
    if (width() > 0) {
        label->resizeToWidth(width());
        label->move(0, 0);
        setFixedHeight(label->height());
    }
}

} // namespace zarya
