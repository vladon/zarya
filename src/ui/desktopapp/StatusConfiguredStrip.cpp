#include "ui/desktopapp/StatusConfiguredStrip.h"

#include "ui/desktopapp/StatusBadgeLibUiEmbed.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"

#include <QMetaObject>
#include <QVBoxLayout>
#include <algorithm>
#include <rpl/rpl.h>

namespace zarya {

StatusConfiguredStrip::StatusConfiguredStrip(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusConfiguredStrip"));

    auto* root = Ui::CreateChild<Ui::RpWidget>(this);
    m_root = root;
    auto* layout = Ui::CreateChild<Ui::VerticalLayout>(root);
    m_layout = layout;

    auto* title = layout->add(
        object_ptr<Ui::FlatLabel>(layout, QString(), st::boxLabel),
        st::boxRowPadding);
    m_title = title;

    const auto badgeRow = layout->add(
        object_ptr<Ui::RpWidget>(layout),
        st::boxRowPadding);
    m_badge = new StatusBadgeLibUiEmbed(badgeRow);
    badgeRow->sizeValue() | rpl::on_next(
        [badgeRow, badge = m_badge](QSize) {
            badge->move(0, 0);
            badgeRow->resize(std::max(badgeRow->width(), badge->width()),
                             std::max(badge->height(), 1));
        },
        badgeRow->lifetime());

    auto* detail = layout->add(
        object_ptr<Ui::FlatLabel>(layout, QString(), st::defaultFlatLabel),
        st::boxRowPadding);
    m_detail = detail;

    const auto primary = layout->add(
        object_ptr<Ui::RoundButton>(
            layout,
            rpl::single(tr("Start")),
            st::defaultBoxButton),
        st::boxRowPadding);
    m_primary = primary;
    primary->setClickedCallback([this] {
        QMetaObject::invokeMethod(
            this,
            [this] {
                if (m_running) {
                    Q_EMIT stopRequested();
                } else {
                    Q_EMIT startRequested();
                }
            },
            Qt::QueuedConnection);
    });

    const auto secondary = layout->add(
        object_ptr<Ui::RoundButton>(
            layout,
            rpl::single(tr("Test")),
            st::defaultBoxButton),
        st::boxRowPadding);
    m_secondary = secondary;
    secondary->setClickedCallback([this] {
        QMetaObject::invokeMethod(
            this,
            [this] {
                if (m_running) {
                    Q_EMIT openLogsRequested();
                } else {
                    Q_EMIT testRequested();
                }
            },
            Qt::QueuedConnection);
    });

    const auto diag = layout->add(
        object_ptr<Ui::RoundButton>(
            layout,
            rpl::single(tr("Create Diagnostics")),
            st::defaultBoxButton),
        st::boxRowPadding);
    m_diag = diag;
    diag->hide();
    diag->setClickedCallback([this] {
        QMetaObject::invokeMethod(
            this,
            [this] { Q_EMIT createDiagnosticsRequested(); },
            Qt::QueuedConnection);
    });

    root->sizeValue() | rpl::on_next(
        [=](QSize size) {
            layout->resizeToWidth(size.width());
            layout->move(0, 0);
            setFixedHeight(layout->height());
        },
        root->lifetime());

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->addWidget(root);
}

void StatusConfiguredStrip::updateModel(const StatusDashboardModel& model)
{
    auto* title = static_cast<Ui::FlatLabel*>(m_title);
    auto* detail = static_cast<Ui::FlatLabel*>(m_detail);
    auto* primary = static_cast<Ui::RoundButton*>(m_primary);
    auto* secondary = static_cast<Ui::RoundButton*>(m_secondary);
    auto* diag = static_cast<Ui::RoundButton*>(m_diag);

    if (model.experimentalRuntimeActive) {
        m_badge->setKind(StatusBadgeKind::Warning);
        m_badge->setBadgeText(tr("Experimental runtime active"));
    } else if (!model.recommendedRuntimeText.isEmpty()) {
        m_badge->setKind(StatusBadgeKind::Ok);
        m_badge->setBadgeText(tr("Recommended: %1").arg(model.recommendedRuntimeText));
    }

    setRunning(model.running);

    if (model.running) {
        title->setText(tr("Runtime: Running — %1").arg(model.runtimeText));
        if (!model.experimentalRuntimeActive) {
            m_badge->setKind(StatusBadgeKind::Running);
            m_badge->setBadgeText(tr("Running"));
        }
        detail->setText(
            tr("Profile: %1\nLocal proxy: %2\nSystem proxy: %3\nRouting: %4")
                .arg(model.profileName, model.localEndpoint, model.systemProxyText,
                     model.routingText));
        primary->setText(rpl::single(tr("Stop")));
        secondary->setText(rpl::single(tr("Open Logs")));
        diag->show();
    } else {
        title->setText(tr("Runtime: Stopped — %1").arg(model.runtimeText));
        if (!model.experimentalRuntimeActive) {
            m_badge->setKind(StatusBadgeKind::Stopped);
            m_badge->setBadgeText(tr("Stopped"));
        }
        detail->setText(
            tr("Selected profile: %1\nRouting: %2\nDNS: %3\nSystem proxy: %4\nCore: %5")
                .arg(model.profileName.isEmpty() ? QStringLiteral("—") : model.profileName,
                     model.routingText, model.dnsText, model.systemProxyText, model.coreText));
        primary->setText(rpl::single(tr("Start")));
        secondary->setText(rpl::single(tr("Test")));
        diag->hide();
    }

    relayout();
}

void StatusConfiguredStrip::relayout()
{
    auto* root = static_cast<Ui::RpWidget*>(m_root);
    auto* layout = static_cast<Ui::VerticalLayout*>(m_layout);
    if (root->width() > 0) {
        layout->resizeToWidth(root->width());
        setFixedHeight(layout->height() + 16);
    }
}

void StatusConfiguredStrip::setRunning(bool running)
{
    m_running = running;
}

} // namespace zarya
