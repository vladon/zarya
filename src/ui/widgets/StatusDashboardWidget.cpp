#include "ui/widgets/StatusDashboardWidget.h"

#include "ui/desktopapp/StatusConfiguredStrip.h"
#include "ui/desktopapp/StatusUnconfiguredPanel.h"

#include <QVBoxLayout>

namespace zarya {

StatusDashboardWidget::StatusDashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusDashboard"));

    m_unconfiguredPanel = new StatusUnconfiguredPanel(this);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::openCoreManagerRequested, this,
            &StatusDashboardWidget::openCoreManagerRequested);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::addProfileRequested, this,
            &StatusDashboardWidget::addProfileRequested);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::addSubscriptionRequested, this,
            &StatusDashboardWidget::addSubscriptionRequested);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::runSetupRequested, this,
            &StatusDashboardWidget::runSetupRequested);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::pasteLinkRequested, this,
            &StatusDashboardWidget::pasteLinkRequested);
    connect(m_unconfiguredPanel, &StatusUnconfiguredPanel::importBackupRequested, this,
            &StatusDashboardWidget::importBackupRequested);

    m_configuredStrip = new StatusConfiguredStrip(this);
    connect(m_configuredStrip, &StatusConfiguredStrip::startRequested, this,
            &StatusDashboardWidget::startRequested);
    connect(m_configuredStrip, &StatusConfiguredStrip::stopRequested, this,
            &StatusDashboardWidget::stopRequested);
    connect(m_configuredStrip, &StatusConfiguredStrip::testRequested, this,
            &StatusDashboardWidget::testRequested);
    connect(m_configuredStrip, &StatusConfiguredStrip::openLogsRequested, this,
            &StatusDashboardWidget::openLogsRequested);
    connect(m_configuredStrip, &StatusConfiguredStrip::createDiagnosticsRequested, this,
            &StatusDashboardWidget::createDiagnosticsRequested);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_unconfiguredPanel);
    root->addWidget(m_configuredStrip);
    m_configuredStrip->hide();
}

void StatusDashboardWidget::updateModel(const StatusDashboardModel& model)
{
    if (!model.configured) {
        showUnconfigured(model);
        return;
    }
    showConfigured(model);
}

void StatusDashboardWidget::showUnconfigured(const StatusDashboardModel& model)
{
    m_unconfiguredPanel->updateModel(model);
    m_unconfiguredPanel->show();
    m_configuredStrip->hide();
}

void StatusDashboardWidget::showConfigured(const StatusDashboardModel& model)
{
    m_unconfiguredPanel->hide();
    m_configuredStrip->show();
    m_configuredStrip->updateModel(model);
}

} // namespace zarya
