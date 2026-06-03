#include "ui/SettingsDialog.h"

#include "storage/AppSettings.h"

#include <QCheckBox>
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

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
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
        new QCheckBox(QStringLiteral("Enable Windows system proxy when profile starts"), this);
    m_autoEnableSystemProxyCheck->setChecked(settings.autoEnableSystemProxyOnStart());
#ifndef Q_OS_WIN
    m_autoEnableSystemProxyCheck->setEnabled(false);
#endif

    m_restoreProxyOnExitCheck =
        new QCheckBox(QStringLiteral("Restore previous proxy settings on stop/exit"), this);
    m_restoreProxyOnExitCheck->setChecked(settings.restoreProxyOnExit());

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
    proxyForm->addRow(QStringLiteral("System proxy endpoint"), m_httpEndpointLabel);
    proxyForm->addRow(QString(), m_autoEnableSystemProxyCheck);
    proxyForm->addRow(QString(), m_restoreProxyOnExitCheck);

    auto* proxyGroup = new QGroupBox(QStringLiteral("Windows system proxy"), this);
    proxyGroup->setLayout(proxyForm);
#ifndef Q_OS_WIN
    proxyGroup->setEnabled(false);
#endif

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
    layout->addWidget(desktopGroup);
    layout->addWidget(buttons);
    resize(580, 540);
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
    settings.setTestUrl(testUrl.toString());
    settings.setTcpTestTimeoutMs(m_tcpTimeoutSpin->value());
    settings.setRealDelayTimeoutMs(m_realDelayTimeoutSpin->value());
    settings.setMaxConcurrentTests(m_maxConcurrentTestsSpin->value());
    settings.setSkipTcpBeforeRealDelay(m_skipTcpBeforeRealDelayCheck->isChecked());
    settings.setMinimizeToTrayOnClose(m_minimizeToTrayOnCloseCheck->isChecked());
    settings.setMinimizeToTrayOnMinimize(m_minimizeToTrayOnMinimizeCheck->isChecked());
    settings.setShowTrayNotifications(m_showTrayNotificationsCheck->isChecked());
    settings.setConfirmExitWhileRunning(m_confirmExitWhileRunningCheck->isChecked());
    return true;
}

} // namespace zarya
