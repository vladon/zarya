#include "ui/widgets/StatusDashboardWidget.h"

#include "ui/widgets/StatusBadge.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

StatusDashboardWidget::StatusDashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("statusDashboard"));

    m_unconfiguredPanel = new QWidget(this);
    auto* unconfiguredTitle = new QLabel(QStringLiteral("Zarya is not configured yet"), m_unconfiguredPanel);
    unconfiguredTitle->setStyleSheet(QStringLiteral("font-weight:bold; font-size:14px;"));
    auto* unconfiguredSteps = new QLabel(
        QStringLiteral("1. Install Xray core\n2. Add a profile or subscription\n3. Start a profile"),
        m_unconfiguredPanel);
    unconfiguredSteps->setWordWrap(true);
    auto* coreBtn = new QPushButton(QStringLiteral("Open Core Manager"), m_unconfiguredPanel);
    auto* profileBtn = new QPushButton(QStringLiteral("Add Profile"), m_unconfiguredPanel);
    auto* subBtn = new QPushButton(QStringLiteral("Add Subscription"), m_unconfiguredPanel);
    auto* setupBtn = new QPushButton(QStringLiteral("Run Setup"), m_unconfiguredPanel);
    auto* pasteBtn = new QPushButton(QStringLiteral("Paste Link"), m_unconfiguredPanel);
    auto* backupBtn = new QPushButton(QStringLiteral("Import Backup"), m_unconfiguredPanel);
    connect(coreBtn, &QPushButton::clicked, this, &StatusDashboardWidget::openCoreManagerRequested);
    connect(profileBtn, &QPushButton::clicked, this, &StatusDashboardWidget::addProfileRequested);
    connect(subBtn, &QPushButton::clicked, this, &StatusDashboardWidget::addSubscriptionRequested);
    connect(setupBtn, &QPushButton::clicked, this, &StatusDashboardWidget::runSetupRequested);
    connect(pasteBtn, &QPushButton::clicked, this, &StatusDashboardWidget::pasteLinkRequested);
    connect(backupBtn, &QPushButton::clicked, this, &StatusDashboardWidget::importBackupRequested);
    auto* unconfiguredLayout = new QVBoxLayout(m_unconfiguredPanel);
    unconfiguredLayout->setContentsMargins(8, 8, 8, 8);
    unconfiguredLayout->addWidget(unconfiguredTitle);
    unconfiguredLayout->addWidget(unconfiguredSteps);
    unconfiguredLayout->addWidget(coreBtn);
    unconfiguredLayout->addWidget(pasteBtn);
    unconfiguredLayout->addWidget(profileBtn);
    unconfiguredLayout->addWidget(subBtn);
    unconfiguredLayout->addWidget(backupBtn);
    unconfiguredLayout->addWidget(setupBtn);

    m_configuredPanel = new QWidget(this);
    m_titleLabel = new QLabel(m_configuredPanel);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight:bold; font-size:14px;"));
    m_runtimeBadge = new StatusBadge(m_configuredPanel);
    m_detailLabel = new QLabel(m_configuredPanel);
    m_detailLabel->setWordWrap(true);
    m_primaryButton = new QPushButton(m_configuredPanel);
    m_secondaryButton = new QPushButton(m_configuredPanel);
    connect(m_primaryButton, &QPushButton::clicked, this, [this]() {
        if (m_primaryButton->text() == QStringLiteral("Stop")) {
            emit stopRequested();
        } else {
            emit startRequested();
        }
    });
    connect(m_secondaryButton, &QPushButton::clicked, this, [this]() {
        const QString text = m_secondaryButton->text();
        if (text.contains(QStringLiteral("Diagnostics"))) {
            emit createDiagnosticsRequested();
        } else if (text.contains(QStringLiteral("Logs"))) {
            emit openLogsRequested();
        } else if (text.contains(QStringLiteral("Test"))) {
            emit testRequested();
        } else if (text.contains(QStringLiteral("Subscriptions"))) {
            emit updateSubscriptionsRequested();
        }
    });
    auto* configuredLayout = new QVBoxLayout(m_configuredPanel);
    configuredLayout->setContentsMargins(8, 8, 8, 8);
    configuredLayout->addWidget(m_titleLabel);
    configuredLayout->addWidget(m_runtimeBadge);
    configuredLayout->addWidget(m_detailLabel);
    configuredLayout->addWidget(m_primaryButton);
    configuredLayout->addWidget(m_secondaryButton);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_unconfiguredPanel);
    root->addWidget(m_configuredPanel);
    m_configuredPanel->hide();
}

void StatusDashboardWidget::updateModel(const StatusDashboardModel& model)
{
    if (!model.configured) {
        showUnconfigured();
        return;
    }
    showConfigured(model);
}

void StatusDashboardWidget::showUnconfigured()
{
    m_unconfiguredPanel->show();
    m_configuredPanel->hide();
}

void StatusDashboardWidget::showConfigured(const StatusDashboardModel& model)
{
    m_unconfiguredPanel->hide();
    m_configuredPanel->show();

    if (model.running) {
        m_titleLabel->setText(QStringLiteral("Runtime: Running"));
        m_runtimeBadge->setKind(StatusBadgeKind::Running);
        m_runtimeBadge->setBadgeText(QStringLiteral("Running"));
        m_detailLabel->setText(
            QStringLiteral("Profile: %1\nLocal HTTP: %2\nLocal SOCKS: %3\nSystem proxy: %4\n"
                           "Routing: %5")
                .arg(model.profileName, model.httpEndpoint, model.socksEndpoint,
                     model.systemProxyText, model.routingText));
        m_primaryButton->setText(QStringLiteral("Stop"));
        m_secondaryButton->setText(QStringLiteral("Open Logs"));
        m_secondaryButton->disconnect();
        connect(m_secondaryButton, &QPushButton::clicked, this,
                &StatusDashboardWidget::openLogsRequested);
        auto* diagBtn = findChild<QPushButton*>(QStringLiteral("diagBtn"));
        if (!diagBtn) {
            diagBtn = new QPushButton(QStringLiteral("Create Diagnostics"), m_configuredPanel);
            diagBtn->setObjectName(QStringLiteral("diagBtn"));
            m_configuredPanel->layout()->addWidget(diagBtn);
            connect(diagBtn, &QPushButton::clicked, this,
                    &StatusDashboardWidget::createDiagnosticsRequested);
        }
        diagBtn->show();
    } else {
        if (auto* diagBtn = findChild<QPushButton*>(QStringLiteral("diagBtn"))) {
            diagBtn->hide();
        }
        m_titleLabel->setText(QStringLiteral("Runtime: Stopped"));
        m_runtimeBadge->setKind(StatusBadgeKind::Stopped);
        m_runtimeBadge->setBadgeText(QStringLiteral("Stopped"));
        m_detailLabel->setText(
            QStringLiteral("Selected profile: %1\nRouting: %2\nDNS: %3\nSystem proxy: %4\nCore: %5")
                .arg(model.profileName.isEmpty() ? QStringLiteral("—") : model.profileName,
                     model.routingText, model.dnsText, model.systemProxyText, model.coreText));
        m_primaryButton->setText(QStringLiteral("Start"));
        m_secondaryButton->setText(QStringLiteral("Test"));
        m_secondaryButton->disconnect();
        connect(m_secondaryButton, &QPushButton::clicked, this,
                &StatusDashboardWidget::testRequested);
    }
}

} // namespace zarya
