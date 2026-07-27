#include "ui/widgets/StatusDashboardWidget.h"

#include "ui/desktopapp/StatusConfiguredStrip.h"

#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

namespace zarya {

StatusDashboardWidget::StatusDashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusDashboard"));

    m_unconfiguredPanel = new QWidget(this);
    auto* unconfiguredTitle = new QLabel(tr("Zarya is not configured yet"), m_unconfiguredPanel);
    {
        QFont font = unconfiguredTitle->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        unconfiguredTitle->setFont(font);
    }
    m_unconfiguredStepsLabel = new QLabel(m_unconfiguredPanel);
    m_unconfiguredStepsLabel->setWordWrap(true);
    auto* coreBtn = new QPushButton(tr("Open Core Manager"), m_unconfiguredPanel);
    auto* profileBtn = new QPushButton(tr("Add Profile"), m_unconfiguredPanel);
    auto* subBtn = new QPushButton(tr("Add Subscription"), m_unconfiguredPanel);
    auto* setupBtn = new QPushButton(tr("Run Setup"), m_unconfiguredPanel);
    auto* pasteBtn = new QPushButton(tr("Paste Link"), m_unconfiguredPanel);
    auto* backupBtn = new QPushButton(tr("Import Backup"), m_unconfiguredPanel);
    connect(coreBtn, &QPushButton::clicked, this, &StatusDashboardWidget::openCoreManagerRequested);
    connect(profileBtn, &QPushButton::clicked, this, &StatusDashboardWidget::addProfileRequested);
    connect(subBtn, &QPushButton::clicked, this, &StatusDashboardWidget::addSubscriptionRequested);
    connect(setupBtn, &QPushButton::clicked, this, &StatusDashboardWidget::runSetupRequested);
    connect(pasteBtn, &QPushButton::clicked, this, &StatusDashboardWidget::pasteLinkRequested);
    connect(backupBtn, &QPushButton::clicked, this, &StatusDashboardWidget::importBackupRequested);
    auto* unconfiguredLayout = new QVBoxLayout(m_unconfiguredPanel);
    unconfiguredLayout->setContentsMargins(8, 8, 8, 8);
    unconfiguredLayout->addWidget(unconfiguredTitle);
    unconfiguredLayout->addWidget(m_unconfiguredStepsLabel);
    unconfiguredLayout->addWidget(coreBtn);
    unconfiguredLayout->addWidget(pasteBtn);
    unconfiguredLayout->addWidget(profileBtn);
    unconfiguredLayout->addWidget(subBtn);
    unconfiguredLayout->addWidget(backupBtn);
    unconfiguredLayout->addWidget(setupBtn);

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
    QStringList steps;
    int step = 1;
    if (!model.xrayInstalled) {
        steps << tr("%1. Install Xray core").arg(step++);
    }
    if (!model.hasProfiles) {
        steps << tr("%1. Add a profile or subscription").arg(step++);
    }
    steps << tr("%1. Start a profile").arg(step);
    m_unconfiguredStepsLabel->setText(steps.join(QLatin1Char('\n')));

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
