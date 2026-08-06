#include "ui/desktopapp/StatusUnconfiguredPanel.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"

#include <QMetaObject>
#include <QStringList>
#include <QVBoxLayout>
#include <rpl/rpl.h>
#include <utility>

namespace zarya {
namespace {

template <typename Callback>
void queueClick(Ui::RoundButton* button, QWidget* context, Callback&& callback)
{
    button->setClickedCallback([context, callback = std::forward<Callback>(callback)] {
        QMetaObject::invokeMethod(context, callback, Qt::QueuedConnection);
    });
}

} // namespace

StatusUnconfiguredPanel::StatusUnconfiguredPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusUnconfiguredPanel"));

    auto* root = Ui::CreateChild<Ui::RpWidget>(this);
    m_root = root;
    auto* layout = Ui::CreateChild<Ui::VerticalLayout>(root);
    m_layout = layout;

    layout->add(
        object_ptr<Ui::FlatLabel>(
            layout,
            StatusDashboardWidget::tr("Zarya is not configured yet"),
            st::boxTitle),
        st::boxRowPadding);

    auto* steps = layout->add(
        object_ptr<Ui::FlatLabel>(layout, QString(), st::boxLabel),
        st::boxRowPadding);
    m_steps = steps;

    auto addButton = [this, layout](
                         const QString& text,
                         const style::RoundButton& buttonStyle,
                         auto signal) {
        auto* button = layout->add(
            object_ptr<Ui::RoundButton>(
                layout,
                rpl::single(text),
                buttonStyle),
            st::boxRowPadding);
        button->setAccessibleName(text);
        queueClick(button, this, [this, signal] { Q_EMIT (this->*signal)(); });
        return button;
    };

    m_coreButton = addButton(
        StatusDashboardWidget::tr("Open Core Manager"),
        st::defaultBoxButton,
        &StatusUnconfiguredPanel::openCoreManagerRequested);
    addButton(
        StatusDashboardWidget::tr("Paste Link"),
        st::defaultBoxButton,
        &StatusUnconfiguredPanel::pasteLinkRequested);
    addButton(
        StatusDashboardWidget::tr("Add Profile"),
        st::defaultBoxButton,
        &StatusUnconfiguredPanel::addProfileRequested);
    addButton(
        StatusDashboardWidget::tr("Add Subscription"),
        st::defaultBoxButton,
        &StatusUnconfiguredPanel::addSubscriptionRequested);
    addButton(
        StatusDashboardWidget::tr("Import Backup"),
        st::defaultBoxButton,
        &StatusUnconfiguredPanel::importBackupRequested);
    addButton(
        StatusDashboardWidget::tr("Run Setup"),
        st::defaultActiveButton,
        &StatusUnconfiguredPanel::runSetupRequested);

    root->sizeValue() | rpl::on_next(
        [this, layout](QSize size) {
            layout->resizeToWidth(size.width());
            layout->move(0, 0);
            setFixedHeight(layout->height() + 16);
        },
        root->lifetime());

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->addWidget(root);
}

void StatusUnconfiguredPanel::updateModel(const StatusDashboardModel& model)
{
    QStringList steps;
    int step = 1;
    if (!model.xrayInstalled) {
        steps << StatusDashboardWidget::tr("%1. Repair or reinstall Zarya (embedded Xray is unavailable)")
                     .arg(step++);
    }
    if (!model.hasProfiles) {
        steps << StatusDashboardWidget::tr("%1. Add a profile or subscription").arg(step++);
    }
    steps << StatusDashboardWidget::tr("%1. Start a profile").arg(step);

    static_cast<Ui::FlatLabel*>(m_steps)->setText(steps.join(QLatin1Char('\n')));
    m_coreButton->setVisible(!model.xrayInstalled);
    relayout();
}

void StatusUnconfiguredPanel::relayout()
{
    auto* root = static_cast<Ui::RpWidget*>(m_root);
    auto* layout = static_cast<Ui::VerticalLayout*>(m_layout);
    if (root->width() > 0) {
        layout->resizeToWidth(root->width());
        setFixedHeight(layout->height() + 16);
    }
}

} // namespace zarya
