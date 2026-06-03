#include "ui/SettingsDialog.h"

#include "platform/ISystemProxyManager.h"
#include "platform/SystemProxyManagerFactory.h"
#include "routing/RoutingManager.h"
#include "storage/AppSettings.h"
#include "ui/RoutingManagerDialog.h"

#if defined(Q_OS_LINUX)
#include "platform/linux/LinuxSystemProxyManager.h"
#endif

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

SettingsDialog::SettingsDialog(RoutingManager& routingManager, QWidget* parent)
    : QDialog(parent)
    , m_routingManager(routingManager)
{
    setWindowTitle(QStringLiteral("Settings"));

    const AppSettings& settings = AppSettings::instance();

    m_xrayPathEdit = new QLineEdit(settings.xrayExecutablePath(), this);
    auto* browseButton = new QPushButton(QStringLiteral("Browse…"), this);
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseXray);

    auto* pathRow = new QHBoxLayout;
    pathRow->addWidget(m_xrayPathEdit);
    pathRow->addWidget(browseButton);

    m_socksPortSpin = new QSpinBox(this);
    m_socksPortSpin->setRange(1, 65535);
    m_socksPortSpin->setValue(settings.socksPort());

    m_httpPortSpin = new QSpinBox(this);
    m_httpPortSpin->setRange(1, 65535);
    m_httpPortSpin->setValue(settings.httpPort());
    connect(m_httpPortSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            &SettingsDialog::updateHttpEndpointLabel);

    m_httpEndpointLabel = new QLabel(this);
    updateHttpEndpointLabel();

    m_autoEnableSystemProxyCheck =
        new QCheckBox(QStringLiteral("Enable system proxy when profile starts"), this);
    m_autoEnableSystemProxyCheck->setChecked(settings.autoEnableSystemProxyOnStart());

    m_restoreProxyOnExitCheck =
        new QCheckBox(QStringLiteral("Restore previous proxy settings on stop/exit"), this);
    m_restoreProxyOnExitCheck->setChecked(settings.restoreProxyOnExit());

    const std::unique_ptr<ISystemProxyManager> proxyManager = SystemProxyManagerFactory::create();
    m_proxyBackendLabel = new QLabel(proxyManager ? proxyManager->backendName() : QString(), this);
    m_proxySupportLabel =
        new QLabel(proxyManager ? proxyManager->supportLevel() : QStringLiteral("unsupported"),
                   this);
    m_proxyLimitationsLabel = new QLabel(proxyManager ? proxyManager->limitations() : QString(), this);
    m_proxyLimitationsLabel->setWordWrap(true);

    m_linuxDesktopLabel = new QLabel(this);
#if defined(Q_OS_LINUX)
    if (auto* linuxManager = dynamic_cast<LinuxSystemProxyManager*>(proxyManager.get())) {
        m_linuxDesktopLabel->setText(
            QStringLiteral("Detected desktop: %1").arg(linuxManager->detectedDesktopName()));
    } else {
        m_linuxDesktopLabel->setText(QStringLiteral("Detected desktop: (unknown)"));
    }
#else
    m_linuxDesktopLabel->hide();
#endif

    m_macApplyAllServicesCheck =
        new QCheckBox(QStringLiteral("Apply proxy to all network services"), this);
    m_macApplyAllServicesCheck->setChecked(settings.macApplyProxyToAllServices());
    m_macPreferredServiceEdit = new QLineEdit(settings.macPreferredNetworkService(), this);
    m_macPreferredServiceEdit->setPlaceholderText(QStringLiteral("e.g. Wi-Fi (optional)"));
#if !defined(Q_OS_MACOS)
    m_macApplyAllServicesCheck->hide();
    m_macPreferredServiceEdit->hide();
#endif

    m_testUrlEdit = new QLineEdit(settings.testUrl(), this);
    m_tcpTimeoutSpin = new QSpinBox(this);
    m_tcpTimeoutSpin->setRange(1000, 60000);
    m_tcpTimeoutSpin->setSuffix(QStringLiteral(" ms"));
    m_tcpTimeoutSpin->setValue(settings.tcpTestTimeoutMs());

    m_realDelayTimeoutSpin = new QSpinBox(this);
    m_realDelayTimeoutSpin->setRange(1000, 60000);
    m_realDelayTimeoutSpin->setSuffix(QStringLiteral(" ms"));
    m_realDelayTimeoutSpin->setValue(settings.realDelayTimeoutMs());

    m_maxConcurrentTestsSpin = new QSpinBox(this);
    m_maxConcurrentTestsSpin->setRange(1, 10);
    m_maxConcurrentTestsSpin->setValue(settings.maxConcurrentTests());

    m_skipTcpBeforeRealDelayCheck =
        new QCheckBox(QStringLiteral("Skip TCP test before real delay"), this);
    m_skipTcpBeforeRealDelayCheck->setChecked(settings.skipTcpBeforeRealDelay());

    m_minimizeToTrayOnCloseCheck =
        new QCheckBox(QStringLiteral("Close button hides to tray"), this);
    m_minimizeToTrayOnCloseCheck->setChecked(settings.minimizeToTrayOnClose());
    m_minimizeToTrayOnMinimizeCheck =
        new QCheckBox(QStringLiteral("Minimize hides to tray"), this);
    m_minimizeToTrayOnMinimizeCheck->setChecked(settings.minimizeToTrayOnMinimize());
    m_showTrayNotificationsCheck =
        new QCheckBox(QStringLiteral("Show tray notifications"), this);
    m_showTrayNotificationsCheck->setChecked(settings.showTrayNotifications());
    m_confirmExitWhileRunningCheck =
        new QCheckBox(QStringLiteral("Confirm exit while core is running"), this);
    m_confirmExitWhileRunningCheck->setChecked(settings.confirmExitWhileRunning());

    auto* coreForm = new QFormLayout;
    coreForm->addRow(QStringLiteral("Xray executable"), pathRow);
    coreForm->addRow(QStringLiteral("Local SOCKS port"), m_socksPortSpin);
    coreForm->addRow(QStringLiteral("Local HTTP port"), m_httpPortSpin);

    auto* coreGroup = new QGroupBox(QStringLiteral("Core"), this);
    coreGroup->setLayout(coreForm);

    auto* proxyForm = new QFormLayout;
    proxyForm->addRow(QStringLiteral("Backend"), m_proxyBackendLabel);
    proxyForm->addRow(QStringLiteral("Support level"), m_proxySupportLabel);
    proxyForm->addRow(QStringLiteral("Limitations"), m_proxyLimitationsLabel);
    proxyForm->addRow(QStringLiteral("System proxy endpoint"), m_httpEndpointLabel);
#if defined(Q_OS_LINUX)
    proxyForm->addRow(QStringLiteral("Desktop"), m_linuxDesktopLabel);
#endif
    proxyForm->addRow(QString(), m_autoEnableSystemProxyCheck);
    proxyForm->addRow(QString(), m_restoreProxyOnExitCheck);
#if defined(Q_OS_MACOS)
    proxyForm->addRow(QString(), m_macApplyAllServicesCheck);
    proxyForm->addRow(QStringLiteral("Preferred network service"), m_macPreferredServiceEdit);
#endif

    auto* proxyGroup = new QGroupBox(QStringLiteral("System proxy"), this);
    proxyGroup->setLayout(proxyForm);

    auto* testingForm = new QFormLayout;
    testingForm->addRow(QStringLiteral("Test URL"), m_testUrlEdit);
    testingForm->addRow(QStringLiteral("TCP timeout"), m_tcpTimeoutSpin);
    testingForm->addRow(QStringLiteral("Real delay timeout"), m_realDelayTimeoutSpin);
    testingForm->addRow(QStringLiteral("Max concurrent tests"), m_maxConcurrentTestsSpin);
    testingForm->addRow(QString(), m_skipTcpBeforeRealDelayCheck);

    auto* testingGroup = new QGroupBox(QStringLiteral("Testing"), this);
    testingGroup->setLayout(testingForm);

    auto* desktopForm = new QFormLayout;
    desktopForm->addRow(QString(), m_minimizeToTrayOnCloseCheck);
    desktopForm->addRow(QString(), m_minimizeToTrayOnMinimizeCheck);
    desktopForm->addRow(QString(), m_showTrayNotificationsCheck);
    desktopForm->addRow(QString(), m_confirmExitWhileRunningCheck);

    auto* desktopGroup = new QGroupBox(QStringLiteral("Desktop behavior"), this);
    desktopGroup->setLayout(desktopForm);

    m_routingProfileCombo = new QComboBox(this);
    refreshRoutingCombo();
    auto* manageRoutingButton = new QPushButton(QStringLiteral("Manage Routing Profiles…"), this);
    connect(manageRoutingButton, &QPushButton::clicked, this,
            &SettingsDialog::onManageRoutingProfiles);

    auto* routingRow = new QHBoxLayout;
    routingRow->addWidget(m_routingProfileCombo, 1);
    routingRow->addWidget(manageRoutingButton);

    auto* routingForm = new QFormLayout;
    routingForm->addRow(QStringLiteral("Active routing profile"), routingRow);

    auto* routingGroup = new QGroupBox(QStringLiteral("Routing"), this);
    routingGroup->setLayout(routingForm);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (validateAndSave()) {
            accept();
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(coreGroup);
    layout->addWidget(proxyGroup);
    layout->addWidget(testingGroup);
    layout->addWidget(routingGroup);
    layout->addWidget(desktopGroup);
    layout->addWidget(buttons);
    resize(620, 720);
}

void SettingsDialog::onBrowseXray()
{
    const QString path =
        QFileDialog::getOpenFileName(this, QStringLiteral("Select Xray executable"), {},
                                   QStringLiteral("Executables (*.exe);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_xrayPathEdit->setText(path);
    }
}

void SettingsDialog::updateHttpEndpointLabel()
{
    m_httpEndpointLabel->setText(
        QStringLiteral("127.0.0.1:%1").arg(m_httpPortSpin->value()));
}

void SettingsDialog::refreshRoutingCombo()
{
    m_routingProfileCombo->clear();
    const QString activeId = m_routingManager.activeProfileId();
    const QVector<RoutingProfile> profiles = m_routingManager.profiles();
    int activeIndex = 0;
    for (int i = 0; i < profiles.size(); ++i) {
        const RoutingProfile& profile = profiles[i];
        m_routingProfileCombo->addItem(profile.name, profile.id);
        if (profile.id == activeId) {
            activeIndex = i;
        }
    }
    m_routingProfileCombo->setCurrentIndex(activeIndex);
}

void SettingsDialog::onManageRoutingProfiles()
{
    RoutingManagerDialog dialog(m_routingManager, {}, this);
    dialog.exec();
    QString error;
    m_routingManager.save(&error);
    refreshRoutingCombo();
}

bool SettingsDialog::validateAndSave()
{
    const QUrl testUrl(m_testUrlEdit->text().trimmed());
    if (!testUrl.isValid()
        || (testUrl.scheme() != QStringLiteral("http") && testUrl.scheme() != QStringLiteral("https"))) {
        QMessageBox::warning(this, QStringLiteral("Settings"),
                             QStringLiteral("Test URL must be a valid http or https URL."));
        return false;
    }

    AppSettings& settings = AppSettings::instance();
    settings.setXrayExecutablePath(m_xrayPathEdit->text().trimmed());
    settings.setSocksPort(m_socksPortSpin->value());
    settings.setHttpPort(m_httpPortSpin->value());
    settings.setAutoEnableSystemProxyOnStart(m_autoEnableSystemProxyCheck->isChecked());
    settings.setRestoreProxyOnExit(m_restoreProxyOnExitCheck->isChecked());
#if defined(Q_OS_MACOS)
    settings.setMacApplyProxyToAllServices(m_macApplyAllServicesCheck->isChecked());
    settings.setMacPreferredNetworkService(m_macPreferredServiceEdit->text());
#endif
    settings.setTestUrl(testUrl.toString());
    settings.setTcpTestTimeoutMs(m_tcpTimeoutSpin->value());
    settings.setRealDelayTimeoutMs(m_realDelayTimeoutSpin->value());
    settings.setMaxConcurrentTests(m_maxConcurrentTestsSpin->value());
    settings.setSkipTcpBeforeRealDelay(m_skipTcpBeforeRealDelayCheck->isChecked());
    settings.setMinimizeToTrayOnClose(m_minimizeToTrayOnCloseCheck->isChecked());
    settings.setMinimizeToTrayOnMinimize(m_minimizeToTrayOnMinimizeCheck->isChecked());
    settings.setShowTrayNotifications(m_showTrayNotificationsCheck->isChecked());
    settings.setConfirmExitWhileRunning(m_confirmExitWhileRunningCheck->isChecked());

    const QString selectedRoutingId = m_routingProfileCombo->currentData().toString();
    if (!selectedRoutingId.isEmpty()) {
        m_routingManager.setActiveProfileId(selectedRoutingId);
        m_routingManager.save();
    }
    return true;
}

} // namespace zarya
