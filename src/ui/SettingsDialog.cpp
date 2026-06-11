#include "ui/SettingsDialog.h"

#include "i18n/LanguageManager.h"
#include "app/BuildInfo.h"
#include "helperclient/HelperProcessManager.h"
#include "service/HelperServiceInstallOptions.h"
#include "service/HelperServiceStatus.h"
#include "service/IHelperServiceManager.h"
#include "storage/AppPaths.h"
#include "killswitch/KillSwitchMode.h"
#include "killswitch/KillSwitchState.h"

#include <QJsonObject>
#include "platform/AutostartManagerFactory.h"
#include "platform/IAutostartManager.h"
#include "platform/ISystemProxyManager.h"
#include "platform/SystemProxyManagerFactory.h"
#include "dns/DnsManager.h"
#include "domain/DnsProfile.h"
#include "routing/RoutingManager.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/AppSettings.h"
#include "ui/DnsManagerDialog.h"
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
#include <QRadioButton>
#include <QSpinBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

SettingsDialog::SettingsDialog(RoutingManager& routingManager, DnsManager& dnsManager,
                               HelperProcessManager* helperManager,
                               IHelperServiceManager* serviceManager, QWidget* parent)
    : QDialog(parent)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
    , m_helperManager(helperManager)
    , m_serviceManager(serviceManager)
{
    setWindowTitle(tr("Settings"));

    const AppSettings& settings = AppSettings::instance();

    m_languageCombo = new QComboBox(this);
    for (const LanguageInfo& lang : LanguageManager::instance().availableLanguages()) {
        m_languageCombo->addItem(lang.nativeName, lang.code);
    }
    const int langIndex =
        m_languageCombo->findData(LanguageManager::instance().currentLanguageCode());
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }

    auto* generalForm = new QFormLayout;
    generalForm->addRow(tr("Language"), m_languageCombo);
    auto* generalGroup = new QGroupBox(tr("General"), this);
    generalGroup->setLayout(generalForm);

    m_xrayPathEdit = new QLineEdit(settings.xrayExecutablePath(), this);
    auto* browseButton = new QPushButton(tr("Browse…"), this);
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
        new QCheckBox(tr("Enable system proxy when profile starts"), this);
    m_autoEnableSystemProxyCheck->setChecked(settings.autoEnableSystemProxyOnStart());

    m_restoreProxyOnExitCheck =
        new QCheckBox(tr("Restore previous proxy settings on stop/exit"), this);
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
            tr("Detected desktop: %1").arg(linuxManager->detectedDesktopName()));
    } else {
        m_linuxDesktopLabel->setText(tr("Detected desktop: (unknown)"));
    }
#else
    m_linuxDesktopLabel->hide();
#endif

    m_macApplyAllServicesCheck =
        new QCheckBox(tr("Apply proxy to all network services"), this);
    m_macApplyAllServicesCheck->setChecked(settings.macApplyProxyToAllServices());
    m_macPreferredServiceEdit = new QLineEdit(settings.macPreferredNetworkService(), this);
    m_macPreferredServiceEdit->setPlaceholderText(tr("e.g. Wi-Fi (optional)"));
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
        new QCheckBox(tr("Skip TCP test before real delay"), this);
    m_skipTcpBeforeRealDelayCheck->setChecked(settings.skipTcpBeforeRealDelay());

    m_minimizeToTrayOnCloseCheck =
        new QCheckBox(tr("Close button hides to tray"), this);
    m_minimizeToTrayOnCloseCheck->setChecked(settings.minimizeToTrayOnClose());
    m_minimizeToTrayOnMinimizeCheck =
        new QCheckBox(tr("Minimize hides to tray"), this);
    m_minimizeToTrayOnMinimizeCheck->setChecked(settings.minimizeToTrayOnMinimize());
    m_showTrayNotificationsCheck =
        new QCheckBox(tr("Show tray notifications"), this);
    m_showTrayNotificationsCheck->setChecked(settings.showTrayNotifications());
    m_confirmExitWhileRunningCheck =
        new QCheckBox(tr("Confirm exit while core is running"), this);
    m_confirmExitWhileRunningCheck->setChecked(settings.confirmExitWhileRunning());

    m_autostartManager = AutostartManagerFactory::create();
    m_autostartBackendLabel =
        new QLabel(m_autostartManager ? m_autostartManager->backendName() : QString(), this);

    m_startAtLoginCheck = new QCheckBox(tr("Start Zarya when I log in"), this);
    const bool osAutostartEnabled =
        m_autostartManager && m_autostartManager->isSupported()
        && m_autostartManager->isEnabled();
    m_startAtLoginCheck->setChecked(settings.startAtLogin() && osAutostartEnabled);
    m_startAtLoginCheck->setEnabled(m_autostartManager && m_autostartManager->isSupported());

    m_startMinimizedToTrayCheck =
        new QCheckBox(tr("Start minimized to tray"), this);
    m_startMinimizedToTrayCheck->setChecked(settings.startMinimizedToTray());

    m_autoStartLastProfileCheck =
        new QCheckBox(tr("Auto-start last used profile"), this);
    m_autoStartLastProfileCheck->setChecked(settings.autoStartLastProfile());

    m_autoEnableProxyAfterAutoStartCheck = new QCheckBox(
        tr("Enable system proxy after auto-starting profile"), this);
    m_autoEnableProxyAfterAutoStartCheck->setChecked(
        settings.autoEnableSystemProxyAfterAutoStart());

    m_autoStartDelaySpin = new QSpinBox(this);
    m_autoStartDelaySpin->setRange(0, 120);
    m_autoStartDelaySpin->setSuffix(QStringLiteral(" s"));
    m_autoStartDelaySpin->setValue(settings.autoStartDelaySeconds());

    auto* coreForm = new QFormLayout;
    coreForm->addRow(tr("Xray executable"), pathRow);
    coreForm->addRow(tr("Local SOCKS port"), m_socksPortSpin);
    coreForm->addRow(tr("Local HTTP port"), m_httpPortSpin);

    auto* coreGroup = new QGroupBox(tr("Cores"), this);
    coreGroup->setLayout(coreForm);

    auto* proxyForm = new QFormLayout;
    proxyForm->addRow(tr("Backend"), m_proxyBackendLabel);
    proxyForm->addRow(tr("Support level"), m_proxySupportLabel);
    proxyForm->addRow(tr("Limitations"), m_proxyLimitationsLabel);
    proxyForm->addRow(tr("System proxy endpoint"), m_httpEndpointLabel);
#if defined(Q_OS_LINUX)
    proxyForm->addRow(tr("Desktop"), m_linuxDesktopLabel);
#endif
    proxyForm->addRow(QString(), m_autoEnableSystemProxyCheck);
    proxyForm->addRow(QString(), m_restoreProxyOnExitCheck);
#if defined(Q_OS_MACOS)
    proxyForm->addRow(QString(), m_macApplyAllServicesCheck);
    proxyForm->addRow(tr("Preferred network service"), m_macPreferredServiceEdit);
#endif

    auto* proxyGroup = new QGroupBox(tr("Proxy Mode"), this);
    proxyGroup->setLayout(proxyForm);

    auto* testingForm = new QFormLayout;
    testingForm->addRow(tr("Test URL"), m_testUrlEdit);
    testingForm->addRow(tr("TCP timeout"), m_tcpTimeoutSpin);
    testingForm->addRow(tr("Real delay timeout"), m_realDelayTimeoutSpin);
    testingForm->addRow(tr("Max concurrent tests"), m_maxConcurrentTestsSpin);
    testingForm->addRow(QString(), m_skipTcpBeforeRealDelayCheck);

    auto* testingGroup = new QGroupBox(tr("Testing"), this);
    testingGroup->setLayout(testingForm);

    auto* desktopForm = new QFormLayout;
    desktopForm->addRow(QString(), m_minimizeToTrayOnCloseCheck);
    desktopForm->addRow(QString(), m_minimizeToTrayOnMinimizeCheck);
    desktopForm->addRow(QString(), m_showTrayNotificationsCheck);
    desktopForm->addRow(QString(), m_confirmExitWhileRunningCheck);

    auto* desktopGroup = new QGroupBox(tr("Desktop behavior"), this);
    desktopGroup->setLayout(desktopForm);

    m_routingProfileCombo = new QComboBox(this);
    refreshRoutingCombo();
    auto* manageRoutingButton = new QPushButton(tr("Manage Routing Profiles…"), this);
    connect(manageRoutingButton, &QPushButton::clicked, this,
            &SettingsDialog::onManageRoutingProfiles);

    auto* routingRow = new QHBoxLayout;
    routingRow->addWidget(m_routingProfileCombo, 1);
    routingRow->addWidget(manageRoutingButton);

    auto* routingForm = new QFormLayout;
    routingForm->addRow(tr("Active routing profile"), routingRow);

    auto* routingGroup = new QGroupBox(tr("Routing"), this);
    routingGroup->setLayout(routingForm);

    m_dnsProfileCombo = new QComboBox(this);
    refreshDnsCombo();
    auto* manageDnsButton = new QPushButton(tr("Manage DNS Profiles…"), this);
    connect(manageDnsButton, &QPushButton::clicked, this, &SettingsDialog::onManageDnsProfiles);

    auto* dnsRow = new QHBoxLayout;
    dnsRow->addWidget(m_dnsProfileCombo, 1);
    dnsRow->addWidget(manageDnsButton);

    auto* dnsForm = new QFormLayout;
    dnsForm->addRow(tr("Active DNS profile"), dnsRow);

    auto* dnsGroup = new QGroupBox(tr("DNS"), this);
    dnsGroup->setLayout(dnsForm);

    auto* startupForm = new QFormLayout;
    startupForm->addRow(tr("Autostart backend"), m_autostartBackendLabel);
    startupForm->addRow(QString(), m_startAtLoginCheck);
    startupForm->addRow(QString(), m_startMinimizedToTrayCheck);
    startupForm->addRow(QString(), m_autoStartLastProfileCheck);
    startupForm->addRow(QString(), m_autoEnableProxyAfterAutoStartCheck);
    startupForm->addRow(tr("Auto-start delay"), m_autoStartDelaySpin);
    if (m_autostartManager && !m_autostartManager->limitations().isEmpty()) {
        auto* autostartLimits = new QLabel(m_autostartManager->limitations(), this);
        autostartLimits->setWordWrap(true);
        startupForm->addRow(tr("Autostart notes"), autostartLimits);
    }

    auto* startupGroup = new QGroupBox(tr("Startup"), this);
    startupGroup->setLayout(startupForm);

    m_enableExperimentalTunCheck =
        new QCheckBox(tr("Enable experimental TUN mode"), this);
    m_enableExperimentalTunCheck->setChecked(settings.enableExperimentalTun());

    m_systemProxyRuntimeRadio =
        new QRadioButton(tr("System proxy via Xray"), this);
    m_tunRuntimeRadio =
        new QRadioButton(tr("TUN via sing-box (experimental)"), this);
    if (settings.runtimeMode() == RuntimeMode::TunSingBoxExperimental) {
        m_tunRuntimeRadio->setChecked(true);
    } else {
        m_systemProxyRuntimeRadio->setChecked(true);
    }

    m_singBoxPathEdit = new QLineEdit(settings.singBoxExecutablePath(), this);
    auto* browseSingBoxButton = new QPushButton(tr("Browse…"), this);
    connect(browseSingBoxButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseSingBox);
    auto* singBoxRow = new QHBoxLayout;
    singBoxRow->addWidget(m_singBoxPathEdit);
    singBoxRow->addWidget(browseSingBoxButton);

    m_tunUseActiveRoutingCheck =
        new QCheckBox(tr("Use active RoutingProfile for TUN"), this);
    m_tunUseActiveRoutingCheck->setChecked(settings.tunUseActiveRoutingProfile());

    m_tunUseActiveDnsCheck =
        new QCheckBox(tr("Use active DnsProfile for TUN"), this);
    m_tunUseActiveDnsCheck->setChecked(settings.tunUseActiveDnsProfile());

    m_tunEnableDnsHijackCheck =
        new QCheckBox(tr("Enable DNS hijack in TUN mode"), this);
    m_tunEnableDnsHijackCheck->setChecked(settings.tunEnableDnsHijack());

    m_tunDnsHijackModeCombo = new QComboBox(this);
    m_tunDnsHijackModeCombo->addItem(tr("Hijack to sing-box DNS"),
                                     static_cast<int>(TunDnsHijackMode::HijackToSingBoxDns));
    m_tunDnsHijackModeCombo->addItem(tr("Disabled"),
                                     static_cast<int>(TunDnsHijackMode::Disabled));
    const int hijackIndex = m_tunDnsHijackModeCombo->findData(
        static_cast<int>(settings.tunDnsHijackMode()));
    if (hijackIndex >= 0) {
        m_tunDnsHijackModeCombo->setCurrentIndex(hijackIndex);
    }

    m_tunDirectGuiRadio =
        new QRadioButton(tr("Run sing-box directly from GUI"), this);
    m_tunHelperRadio =
        new QRadioButton(tr("Use zarya-helper experimental"), this);
    if (settings.tunPrivilegeMode() == TunPrivilegeMode::HelperExperimental) {
        m_tunHelperRadio->setChecked(true);
    } else {
        m_tunDirectGuiRadio->setChecked(true);
    }

    m_helperBackendLabel = new QLabel(this);
    m_helperServiceStatusLabel = new QLabel(this);
    m_helperStatusLabel = new QLabel(m_helperManager ? m_helperManager->statusText()
                                                     : tr("Helper unavailable"),
                                     this);
    m_installServiceButton = new QPushButton(tr("Install"), this);
    m_uninstallServiceButton = new QPushButton(tr("Uninstall"), this);
    m_startServiceButton = new QPushButton(tr("Start Service"), this);
    m_stopServiceButton = new QPushButton(tr("Stop Service"), this);
    m_restartServiceButton = new QPushButton(tr("Restart Service"), this);
    m_startHelperButton = new QPushButton(tr("Start Manual Helper"), this);
    m_connectHelperButton = new QPushButton(tr("Connect"), this);
    m_checkHelperStatusButton = new QPushButton(tr("Check Status"), this);
    m_serviceSelfTestButton = new QPushButton(tr("Run Self-Test"), this);
    m_serviceRecoveryButton = new QPushButton(tr("Show Recovery Instructions"), this);
    m_recoverKillSwitchOnUninstallCheck =
        new QCheckBox(tr("Also recover/remove Zarya kill switch rules on uninstall"), this);

    connect(m_installServiceButton, &QPushButton::clicked, this, &SettingsDialog::onInstallService);
    connect(m_uninstallServiceButton, &QPushButton::clicked, this, &SettingsDialog::onUninstallService);
    connect(m_startServiceButton, &QPushButton::clicked, this, &SettingsDialog::onStartService);
    connect(m_stopServiceButton, &QPushButton::clicked, this, &SettingsDialog::onStopService);
    connect(m_restartServiceButton, &QPushButton::clicked, this, &SettingsDialog::onRestartService);
    connect(m_startHelperButton, &QPushButton::clicked, this, &SettingsDialog::onStartHelper);
    connect(m_connectHelperButton, &QPushButton::clicked, this, &SettingsDialog::onConnectHelper);
    connect(m_checkHelperStatusButton, &QPushButton::clicked, this,
            &SettingsDialog::onCheckHelperStatus);
    connect(m_serviceSelfTestButton, &QPushButton::clicked, this, &SettingsDialog::onServiceSelfTest);
    connect(m_serviceRecoveryButton, &QPushButton::clicked, this,
            &SettingsDialog::onShowServiceRecovery);
    if (m_helperManager) {
        connect(m_helperManager, &HelperProcessManager::connectionStateChanged, this,
                [this]() {
                    m_helperStatusLabel->setText(m_helperManager->statusText());
                    refreshHelperServiceUi();
                });
    }
    if (m_serviceManager) {
        connect(m_serviceManager, &IHelperServiceManager::statusChanged, this,
                &SettingsDialog::refreshHelperServiceUi);
    }

    auto* serviceButtonsRow = new QHBoxLayout;
    serviceButtonsRow->addWidget(m_installServiceButton);
    serviceButtonsRow->addWidget(m_uninstallServiceButton);
    serviceButtonsRow->addWidget(m_startServiceButton);
    serviceButtonsRow->addWidget(m_stopServiceButton);
    serviceButtonsRow->addWidget(m_restartServiceButton);

    auto* helperButtonsRow = new QHBoxLayout;
    helperButtonsRow->addWidget(m_startHelperButton);
    helperButtonsRow->addWidget(m_connectHelperButton);
    helperButtonsRow->addWidget(m_checkHelperStatusButton);
    helperButtonsRow->addWidget(m_serviceSelfTestButton);
    helperButtonsRow->addWidget(m_serviceRecoveryButton);

    QString helperWarningText =
        tr("Installing the helper requires administrator/root privileges.\n"
           "The helper can start TUN mode and manage kill switch rules.\n"
           "Only install it from a trusted Zarya build.");
    if (!BuildInfo::isSigned()) {
        helperWarningText +=
            QLatin1Char('\n')
            + tr("This build is unsigned. Installing privileged helper from unsigned builds is "
                 "not recommended for production use.");
    }
    m_helperServiceWarningLabel = new QLabel(helperWarningText, this);
    m_helperServiceWarningLabel->setWordWrap(true);

    auto* tunWarnings = new QLabel(
        tr("TUN mode requires sing-box and may require zarya-helper. System-proxy mode does not "
           "require the helper service."),
        this);
    tunWarnings->setWordWrap(true);

    auto* experimentalForm = new QFormLayout;
    experimentalForm->addRow(QString(), m_enableExperimentalTunCheck);
    experimentalForm->addRow(QString(), m_systemProxyRuntimeRadio);
    experimentalForm->addRow(QString(), m_tunRuntimeRadio);
    experimentalForm->addRow(tr("sing-box executable"), singBoxRow);
    experimentalForm->addRow(QString(), m_tunUseActiveRoutingCheck);
    experimentalForm->addRow(QString(), m_tunUseActiveDnsCheck);
    experimentalForm->addRow(QString(), m_tunEnableDnsHijackCheck);
    experimentalForm->addRow(tr("TUN DNS hijack mode"), m_tunDnsHijackModeCombo);
    experimentalForm->addRow(tr("TUN privilege mode"), m_tunDirectGuiRadio);
    experimentalForm->addRow(QString(), m_tunHelperRadio);
    experimentalForm->addRow(tr("Privileged helper backend"), m_helperBackendLabel);
    experimentalForm->addRow(tr("Service status"), m_helperServiceStatusLabel);
    experimentalForm->addRow(tr("IPC connection"), m_helperStatusLabel);
    experimentalForm->addRow(QString(), serviceButtonsRow);
    experimentalForm->addRow(QString(), m_recoverKillSwitchOnUninstallCheck);
    experimentalForm->addRow(QString(), helperButtonsRow);
    experimentalForm->addRow(QString(), m_helperServiceWarningLabel);

    m_tunRequireLocalRuleSetsCheck =
        new QCheckBox(tr("Require local .srs rule sets before starting TUN"), this);
    m_tunRequireLocalRuleSetsCheck->setChecked(settings.tunRequireLocalRuleSets());

    m_ruleSetDirLabel = new QLabel(AppPaths::singBoxRuleSetDir(), this);
    m_ruleSetDirLabel->setWordWrap(true);
    m_ruleSetDirLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* ruleSetNote = new QLabel(
        tr("Manage rule sets from Tools → sing-box Rule Sets. Xray geoip.dat/geosite.dat "
           "are separate from sing-box .srs files."),
        this);
    ruleSetNote->setWordWrap(true);

    experimentalForm->addRow(tr("Rule sets"), m_tunRequireLocalRuleSetsCheck);
    experimentalForm->addRow(tr("Rule-set directory"), m_ruleSetDirLabel);
    experimentalForm->addRow(QString(), ruleSetNote);
    experimentalForm->addRow(QString(), tunWarnings);

    auto* experimentalGroup = new QGroupBox(
        tr("Experimental (TUN · helper · kill switch)"), this);
    experimentalGroup->setLayout(experimentalForm);

    m_allowCoreUpdateWithoutChecksumCheck =
        new QCheckBox(tr("Allow installing core archives without checksum verification"),
                      this);
    m_allowCoreUpdateWithoutChecksumCheck->setChecked(settings.allowCoreUpdateWithoutChecksum());

    m_allowManageExternalCorePathsCheck =
        new QCheckBox(tr("Allow managing cores outside Zarya-managed directory"), this);
    m_allowManageExternalCorePathsCheck->setChecked(settings.allowManageExternalCorePaths());

    m_coreBackupRetentionSpin = new QSpinBox(this);
    m_coreBackupRetentionSpin->setRange(1, 10);
    m_coreBackupRetentionSpin->setValue(settings.coreBackupRetentionCount());

    m_githubApiTimeoutSpin = new QSpinBox(this);
    m_githubApiTimeoutSpin->setRange(5, 120);
    m_githubApiTimeoutSpin->setSuffix(QStringLiteral(" s"));
    m_githubApiTimeoutSpin->setValue(settings.githubApiTimeoutSeconds());

    m_checkCoreUpdatesOnStartupCheck =
        new QCheckBox(tr("Check core updates on startup"), this);
    m_checkCoreUpdatesOnStartupCheck->setChecked(settings.checkCoreUpdatesOnStartup());

    auto* coreUpdatesForm = new QFormLayout;
    coreUpdatesForm->addRow(QString(), m_allowCoreUpdateWithoutChecksumCheck);
    coreUpdatesForm->addRow(QString(), m_allowManageExternalCorePathsCheck);
    coreUpdatesForm->addRow(tr("Backup retention"), m_coreBackupRetentionSpin);
    coreUpdatesForm->addRow(tr("GitHub API timeout"), m_githubApiTimeoutSpin);
    coreUpdatesForm->addRow(QString(), m_checkCoreUpdatesOnStartupCheck);

    auto* coreUpdatesGroup = new QGroupBox(tr("Core updates"), this);
    coreUpdatesGroup->setLayout(coreUpdatesForm);

    m_enableKillSwitchCheck =
        new QCheckBox(tr("Enable experimental kill switch"), this);
    m_enableKillSwitchCheck->setChecked(settings.enableExperimentalKillSwitch());

    m_killSwitchModeLabel =
        new QLabel(tr("Mode: TUN only experimental"), this);

    m_killSwitchAllowLanCheck =
        new QCheckBox(tr("Allow LAN/private networks"), this);
    m_killSwitchAllowLanCheck->setChecked(settings.killSwitchAllowLan());

    m_killSwitchAllowLoopbackCheck =
        new QCheckBox(tr("Allow loopback"), this);
    m_killSwitchAllowLoopbackCheck->setChecked(settings.killSwitchAllowLoopback());

    m_killSwitchAllowProxyCheck =
        new QCheckBox(tr("Allow traffic to selected proxy server"), this);
    m_killSwitchAllowProxyCheck->setChecked(true);
    m_killSwitchAllowProxyCheck->setEnabled(false);

    m_killSwitchAutoDisableOnStopCheck =
        new QCheckBox(tr("Disable kill switch on clean Stop"), this);
    m_killSwitchAutoDisableOnStopCheck->setChecked(settings.killSwitchAutoDisableOnCleanStop());

    m_killSwitchKeepActiveAfterStopCheck =
        new QCheckBox(tr("Keep kill switch active after Stop"), this);
    m_killSwitchKeepActiveAfterStopCheck->setChecked(
        !settings.killSwitchAutoDisableOnCleanStop());

    connect(m_killSwitchAutoDisableOnStopCheck, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) {
                    m_killSwitchKeepActiveAfterStopCheck->setChecked(false);
                }
            });
    connect(m_killSwitchKeepActiveAfterStopCheck, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) {
                    m_killSwitchAutoDisableOnStopCheck->setChecked(false);
                }
            });

#if defined(Q_OS_LINUX)
    m_killSwitchBackendLabel = new QLabel(
        tr("Kill switch backend: nftables PoC (table inet zarya)"), this);
#elif defined(Q_OS_WIN)
    m_killSwitchBackendLabel = new QLabel(
        tr("Kill switch backend: Windows WFP PoC (requires Administrator helper)"),
        this);
#elif defined(Q_OS_MACOS)
    m_killSwitchBackendLabel = new QLabel(
        tr("Kill switch backend: unsupported in 0.16 — PF is not a stable public API for "
           "third-party apps."),
        this);
#else
    m_killSwitchBackendLabel = new QLabel(tr("Kill switch backend: unsupported"),
                                          this);
#endif
    m_killSwitchBackendLabel->setWordWrap(true);

    m_killSwitchWarningLabel = new QLabel(
        tr("Experimental kill switch changes firewall/routing rules. A bug may block network "
           "access until rules are removed manually. Requires zarya-helper mode. Use only if you "
           "understand the recovery procedure."),
        this);
    m_killSwitchWarningLabel->setWordWrap(true);

    m_testKillSwitchButton = new QPushButton(tr("Test Support"), this);
    m_enableKillSwitchButton = new QPushButton(tr("How it works"), this);
    m_disableKillSwitchButton = new QPushButton(tr("Disable Now"), this);
    m_killSwitchRecoveryButton =
        new QPushButton(tr("Show Recovery Instructions"), this);
    connect(m_testKillSwitchButton, &QPushButton::clicked, this,
            &SettingsDialog::onTestKillSwitchSupport);
    connect(m_enableKillSwitchButton, &QPushButton::clicked, this,
            &SettingsDialog::onEnableKillSwitchNow);
    connect(m_disableKillSwitchButton, &QPushButton::clicked, this,
            &SettingsDialog::onDisableKillSwitchNow);
    connect(m_killSwitchRecoveryButton, &QPushButton::clicked, this,
            &SettingsDialog::onShowKillSwitchRecovery);

    auto* killSwitchButtonsRow = new QHBoxLayout;
    killSwitchButtonsRow->addWidget(m_testKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_enableKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_disableKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_killSwitchRecoveryButton);

    auto* killSwitchForm = new QFormLayout;
    killSwitchForm->addRow(QString(), m_enableKillSwitchCheck);
    killSwitchForm->addRow(QString(), m_killSwitchModeLabel);
    killSwitchForm->addRow(QString(), m_killSwitchAllowLanCheck);
    killSwitchForm->addRow(QString(), m_killSwitchAllowLoopbackCheck);
    killSwitchForm->addRow(QString(), m_killSwitchAllowProxyCheck);
    killSwitchForm->addRow(QString(), m_killSwitchAutoDisableOnStopCheck);
    killSwitchForm->addRow(QString(), m_killSwitchKeepActiveAfterStopCheck);
    killSwitchForm->addRow(QString(), m_killSwitchBackendLabel);
    killSwitchForm->addRow(QString(), m_killSwitchWarningLabel);
    killSwitchForm->addRow(QString(), killSwitchButtonsRow);

    auto* killSwitchGroup = new QGroupBox(
        tr("Kill Switch — Experimental · Requires helper · Linux/Windows PoC"), this);
    killSwitchGroup->setLayout(killSwitchForm);

    const auto updateRuntimeControls = [this]() {
        const bool enabled = m_enableExperimentalTunCheck->isChecked();
        m_systemProxyRuntimeRadio->setEnabled(enabled);
        m_tunRuntimeRadio->setEnabled(enabled);
        m_singBoxPathEdit->setEnabled(enabled);
        m_tunUseActiveRoutingCheck->setEnabled(enabled);
        m_tunUseActiveDnsCheck->setEnabled(enabled);
        m_tunEnableDnsHijackCheck->setEnabled(enabled);
        m_tunDnsHijackModeCombo->setEnabled(enabled && m_tunEnableDnsHijackCheck->isChecked());
        m_tunDirectGuiRadio->setEnabled(enabled);
        m_tunHelperRadio->setEnabled(enabled);
        m_tunRequireLocalRuleSetsCheck->setEnabled(enabled);
        const bool helperUi = enabled && m_helperManager != nullptr;
        const bool serviceUi = enabled && m_serviceManager != nullptr;
        m_startHelperButton->setEnabled(helperUi);
        m_connectHelperButton->setEnabled(helperUi);
        m_checkHelperStatusButton->setEnabled(helperUi);
        m_installServiceButton->setEnabled(serviceUi);
        m_uninstallServiceButton->setEnabled(serviceUi);
        m_startServiceButton->setEnabled(serviceUi);
        m_stopServiceButton->setEnabled(serviceUi);
        m_restartServiceButton->setEnabled(serviceUi);
        m_serviceSelfTestButton->setEnabled(serviceUi || helperUi);
        m_serviceRecoveryButton->setEnabled(serviceUi || helperUi);
        m_recoverKillSwitchOnUninstallCheck->setEnabled(serviceUi);
        updateKillSwitchControls();
    };
    connect(m_tunEnableDnsHijackCheck, &QCheckBox::toggled, this, updateRuntimeControls);
    updateRuntimeControls();
    refreshHelperServiceUi();
    connect(m_enableExperimentalTunCheck, &QCheckBox::toggled, this, updateRuntimeControls);
    connect(m_enableKillSwitchCheck, &QCheckBox::toggled, this,
            &SettingsDialog::updateKillSwitchControls);
    connect(m_tunHelperRadio, &QRadioButton::toggled, this, &SettingsDialog::updateKillSwitchControls);
    updateKillSwitchControls();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (validateAndSave()) {
            accept();
        }
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(generalGroup);
    layout->addWidget(coreGroup);
    layout->addWidget(proxyGroup);
    layout->addWidget(routingGroup);
    layout->addWidget(dnsGroup);
    layout->addWidget(startupGroup);
    layout->addWidget(desktopGroup);
    layout->addWidget(coreUpdatesGroup);
    layout->addWidget(testingGroup);
    layout->addWidget(experimentalGroup);
    layout->addWidget(killSwitchGroup);
    layout->addWidget(buttons);
    resize(620, 1080);
}

void SettingsDialog::onBrowseSingBox()
{
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Select sing-box executable"), {},
                                   QStringLiteral("Executables (*.exe);;All files (*.*)"));
    if (!path.isEmpty()) {
        m_singBoxPathEdit->setText(path);
    }
}

bool SettingsDialog::confirmTunWarningIfNeeded()
{
    if (AppSettings::instance().tunWarningAccepted()) {
        return true;
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Experimental TUN mode"));
    box.setText(tr(
        "TUN mode is experimental. It may change network routes and DNS behavior.\n\n"
        "If it fails, Zarya will attempt to stop sing-box and restore state, but this mode is "
        "not production-ready yet.\n\n"
        "Kill switch is experimental and requires zarya-helper mode."));
    QPushButton* enableButton = box.addButton(tr("Enable Experimental TUN"),
                                              QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != enableButton) {
        return false;
    }
    AppSettings::instance().setTunWarningAccepted(true);
    return true;
}

void SettingsDialog::onBrowseXray()
{
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Select Xray executable"), {},
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

void SettingsDialog::refreshDnsCombo()
{
    m_dnsProfileCombo->clear();
    const QString activeId = m_dnsManager.activeProfileId();
    const QVector<DnsProfile> profiles = m_dnsManager.profiles();
    int activeIndex = 0;
    for (int i = 0; i < profiles.size(); ++i) {
        const DnsProfile& profile = profiles[i];
        m_dnsProfileCombo->addItem(profile.name, profile.id);
        if (profile.id == activeId) {
            activeIndex = i;
        }
    }
    m_dnsProfileCombo->setCurrentIndex(activeIndex);
}

void SettingsDialog::onManageDnsProfiles()
{
    DnsManagerDialog dialog(m_dnsManager, {}, this);
    dialog.exec();
    QString error;
    m_dnsManager.save(&error);
    refreshDnsCombo();
}

bool SettingsDialog::validateAndSave()
{
    const QUrl testUrl(m_testUrlEdit->text().trimmed());
    if (!testUrl.isValid()
        || (testUrl.scheme() != QStringLiteral("http") && testUrl.scheme() != QStringLiteral("https"))) {
        QMessageBox::warning(this, tr("Settings"),
                             tr("Test URL must be a valid http or https URL."));
        return false;
    }

    bool languageChanged = false;
    const QString selectedLanguage = m_languageCombo->currentData().toString();
    if (selectedLanguage != LanguageManager::instance().currentLanguageCode()) {
        QString langError;
        if (!LanguageManager::instance().setLanguage(selectedLanguage, &langError)) {
            QMessageBox::warning(this, tr("Settings"), langError);
            return false;
        }
        languageChanged = true;
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

    settings.setStartMinimizedToTray(m_startMinimizedToTrayCheck->isChecked());
    settings.setAutoStartLastProfile(m_autoStartLastProfileCheck->isChecked());
    settings.setAutoEnableSystemProxyAfterAutoStart(
        m_autoEnableProxyAfterAutoStartCheck->isChecked());
    settings.setAutoStartDelaySeconds(m_autoStartDelaySpin->value());

    if (m_autostartManager && m_autostartManager->isSupported()) {
        const bool wantAutostart = m_startAtLoginCheck->isChecked();
        QString autostartError;
        if (wantAutostart) {
            if (!m_autostartManager->enable({QStringLiteral("--minimized")}, &autostartError)) {
                QMessageBox::warning(this, tr("Autostart"), autostartError);
                return false;
            }
        } else if (m_autostartManager->isEnabled()
                   && !m_autostartManager->disable(&autostartError)) {
            QMessageBox::warning(this, tr("Autostart"), autostartError);
            return false;
        }
        settings.setStartAtLogin(wantAutostart);
    }

    const QString selectedRoutingId = m_routingProfileCombo->currentData().toString();
    if (!selectedRoutingId.isEmpty()) {
        m_routingManager.setActiveProfileId(selectedRoutingId);
        m_routingManager.save();
    }

    const QString selectedDnsId = m_dnsProfileCombo->currentData().toString();
    if (!selectedDnsId.isEmpty()) {
        m_dnsManager.setActiveProfileId(selectedDnsId);
        m_dnsManager.save();
    }

    const bool wantExperimentalTun = m_enableExperimentalTunCheck->isChecked();
    const RuntimeMode selectedMode = m_tunRuntimeRadio->isChecked()
                                         ? RuntimeMode::TunSingBoxExperimental
                                         : RuntimeMode::SystemProxyXray;
    if (wantExperimentalTun || selectedMode == RuntimeMode::TunSingBoxExperimental) {
        if (!confirmTunWarningIfNeeded()) {
            return false;
        }
    }

    settings.setEnableExperimentalTun(wantExperimentalTun);
    settings.setRuntimeMode(selectedMode);
    settings.setSingBoxExecutablePath(m_singBoxPathEdit->text());
    settings.setTunUseActiveRoutingProfile(m_tunUseActiveRoutingCheck->isChecked());
    settings.setTunUseActiveDnsProfile(m_tunUseActiveDnsCheck->isChecked());
    settings.setTunEnableDnsHijack(m_tunEnableDnsHijackCheck->isChecked());
    settings.setTunDnsHijackMode(static_cast<TunDnsHijackMode>(
        m_tunDnsHijackModeCombo->currentData().toInt()));
    settings.setTunPrivilegeMode(m_tunHelperRadio->isChecked()
                                     ? TunPrivilegeMode::HelperExperimental
                                     : TunPrivilegeMode::DirectFromGui);
    settings.setTunRequireLocalRuleSets(m_tunRequireLocalRuleSetsCheck->isChecked());

    const bool killSwitchEnabled = m_enableKillSwitchCheck->isChecked();
    settings.setEnableExperimentalKillSwitch(killSwitchEnabled);
    settings.setKillSwitchMode(killSwitchEnabled ? KillSwitchMode::TunOnlyExperimental
                                                 : KillSwitchMode::Disabled);
    settings.setKillSwitchAllowLan(m_killSwitchAllowLanCheck->isChecked());
    settings.setKillSwitchAllowLoopback(m_killSwitchAllowLoopbackCheck->isChecked());
    settings.setKillSwitchBlockWhenTunStopped(true);
    settings.setKillSwitchAutoDisableOnCleanStop(
        m_killSwitchAutoDisableOnStopCheck->isChecked());

    settings.setAllowCoreUpdateWithoutChecksum(m_allowCoreUpdateWithoutChecksumCheck->isChecked());
    settings.setAllowManageExternalCorePaths(m_allowManageExternalCorePathsCheck->isChecked());
    settings.setCoreBackupRetentionCount(m_coreBackupRetentionSpin->value());
    settings.setGithubApiTimeoutSeconds(m_githubApiTimeoutSpin->value());
    settings.setCheckCoreUpdatesOnStartup(m_checkCoreUpdatesOnStartupCheck->isChecked());

    if (languageChanged) {
        QMessageBox::information(
            this, tr("Settings"),
            tr("Language will be fully applied after restart."));
    }

    return true;
}

void SettingsDialog::updateKillSwitchControls()
{
    const bool enabled = m_enableKillSwitchCheck->isChecked();
    const bool helperMode = m_tunHelperRadio->isChecked();
    m_killSwitchModeLabel->setEnabled(enabled);
    m_killSwitchAllowLanCheck->setEnabled(enabled);
    m_killSwitchAllowLoopbackCheck->setEnabled(enabled);
    m_killSwitchAutoDisableOnStopCheck->setEnabled(enabled);
    m_killSwitchKeepActiveAfterStopCheck->setEnabled(enabled);
    const bool helperUi = enabled && helperMode && m_helperManager != nullptr;
    m_testKillSwitchButton->setEnabled(helperUi);
    m_enableKillSwitchButton->setEnabled(helperUi);
    m_disableKillSwitchButton->setEnabled(helperUi);
    m_killSwitchRecoveryButton->setEnabled(true);
}

void SettingsDialog::onTestKillSwitchSupport()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->connectToHelper(&error)) {
        QMessageBox::warning(this, tr("Kill switch"), error);
        return;
    }
    QJsonObject payload;
    if (!m_helperManager->killSwitchCheckSupport(&payload, &error)) {
        QMessageBox::warning(this, tr("Kill switch"), error);
        return;
    }
    QMessageBox::information(
        this, tr("Kill switch support"),
        tr("Backend: %1\nPrivileged: %2\nSupported: %3\n\n%4")
            .arg(payload.value(QStringLiteral("backend")).toString(),
                 payload.value(QStringLiteral("privileged")).toBool() ? tr("yes") : tr("no"),
                 payload.value(QStringLiteral("supported")).toBool() ? tr("yes") : tr("no"),
                 payload.value(QStringLiteral("lastError")).toString()));
}

void SettingsDialog::onEnableKillSwitchNow()
{
#if defined(Q_OS_WIN)
    QMessageBox::warning(
        this, tr("Kill switch"),
        tr("The Windows WFP kill switch is experimental.\n\n"
           "It will install temporary Zarya-owned WFP filters to block outbound connections "
           "except loopback and the selected proxy server.\n\n"
           "zarya-helper must be running as Administrator.\n\n"
           "Kill switch is enabled automatically when you start TUN mode with kill switch "
           "enabled."));
#else
    QMessageBox::information(
        this, tr("Kill switch"),
        tr("Kill switch is enabled automatically when you start TUN mode with kill switch "
           "enabled.\n\nSelect a profile and press Start, or use a running TUN session."));
#endif
}

void SettingsDialog::onDisableKillSwitchNow()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->connectToHelper(&error)) {
        QMessageBox::warning(this, tr("Kill switch"), error);
        return;
    }
    if (!m_helperManager->killSwitchDisable(&error)) {
        QMessageBox::warning(this, tr("Kill switch"), error);
        return;
    }
    QMessageBox::information(this, tr("Kill switch"),
                             tr("Kill switch disabled."));
}

void SettingsDialog::onShowKillSwitchRecovery()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Kill switch recovery"));
    box.setText(HelperProcessManager::recoveryInstructionsText());
    box.setStandardButtons(QMessageBox::Close);
    box.exec();
}

void SettingsDialog::onStartHelper()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->startHelperDevMode(&error)) {
        QMessageBox::warning(this, tr("Helper"), error);
    }
    m_helperStatusLabel->setText(m_helperManager->statusText());
}

void SettingsDialog::onConnectHelper()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->connectToHelper(&error)) {
        QMessageBox::warning(this, tr("Helper"), error);
    }
    m_helperStatusLabel->setText(m_helperManager->statusText());
}

void SettingsDialog::onCheckHelperStatus()
{
    if (!m_helperManager) {
        return;
    }
    QJsonObject payload;
    QString error;
    if (!m_helperManager->status(&payload, &error)) {
        QMessageBox::warning(this, tr("Helper"), error);
        return;
    }
    m_helperStatusLabel->setText(
        tr("running=%1, pid=%2")
            .arg(payload.value(QStringLiteral("running")).toBool() ? tr("yes") : tr("no"))
            .arg(payload.value(QStringLiteral("pid")).toInt()));
}

void SettingsDialog::refreshHelperServiceUi()
{
    if (!m_helperBackendLabel || !m_helperServiceStatusLabel) {
        return;
    }
    if (!m_serviceManager) {
        m_helperBackendLabel->setText(tr("Manual helper"));
        m_helperServiceStatusLabel->setText(tr("Unavailable"));
        return;
    }

    const HelperServiceStatus status = m_serviceManager->status();
    m_helperBackendLabel->setText(status.backend);
    QString statusText = helperServiceInstallStateToString(status.state);
    if (m_helperManager
        && m_helperManager->connectionState() == HelperConnectionState::Connected) {
        statusText += tr(" · Connected");
    }
    if (!status.lastError.isEmpty()) {
        statusText += QStringLiteral(" — %1").arg(status.lastError);
    }
    m_helperServiceStatusLabel->setText(statusText);
}

void SettingsDialog::onInstallService()
{
    if (!m_serviceManager || !m_helperManager) {
        return;
    }
    HelperServiceInstallOptions options =
        HelperServiceInstallOptions::defaultsForCurrentApp(m_helperManager->helperExecutablePath());
    QString error;
    if (!m_serviceManager->install(options, &error)) {
        QMessageBox msg(this);
        msg.setWindowTitle(tr("Install helper service"));
        msg.setText(tr("Installing Zarya Helper service requires Administrator privileges."));
        msg.setInformativeText(error);
        msg.setStandardButtons(QMessageBox::Close);
        msg.exec();
        refreshHelperServiceUi();
        return;
    }
    refreshHelperServiceUi();
}

void SettingsDialog::onUninstallService()
{
    if (!m_serviceManager) {
        return;
    }
    if (m_recoverKillSwitchOnUninstallCheck->isChecked() && m_helperManager) {
        QString recoverError;
        m_helperManager->killSwitchRecover(true, &recoverError);
    }
    QString error;
    if (!m_serviceManager->uninstall(m_recoverKillSwitchOnUninstallCheck->isChecked(), &error)) {
        QMessageBox::warning(this, tr("Uninstall helper service"), error);
    }
    refreshHelperServiceUi();
}

void SettingsDialog::onStartService()
{
    if (!m_serviceManager) {
        return;
    }
    QString error;
    if (!m_serviceManager->start(&error)) {
        QMessageBox::warning(this, tr("Start helper service"), error);
    }
    refreshHelperServiceUi();
}

void SettingsDialog::onStopService()
{
    if (!m_serviceManager) {
        return;
    }
    QString error;
    if (!m_serviceManager->stop(&error)) {
        QMessageBox::warning(this, tr("Stop helper service"), error);
    }
    refreshHelperServiceUi();
}

void SettingsDialog::onRestartService()
{
    if (!m_serviceManager) {
        return;
    }
    QString error;
    if (!m_serviceManager->restart(&error)) {
        QMessageBox::warning(this, tr("Restart helper service"), error);
    }
    refreshHelperServiceUi();
}

void SettingsDialog::onServiceSelfTest()
{
    if (!m_helperManager) {
        return;
    }
    QProcess process;
    process.start(m_helperManager->helperExecutablePath(),
                  {QStringLiteral("--service-self-test"),
                   QStringLiteral("--allowed-runtime-dir"),
                   AppPaths::runtimeDir(),
                   QStringLiteral("--allowed-core-dir"),
                   AppPaths::singBoxCoreDir()});
    process.waitForFinished(15000);
    QMessageBox box(this);
    box.setWindowTitle(tr("Helper self-test"));
    box.setText(QString::fromUtf8(process.readAllStandardOutput()));
    if (process.exitCode() != 0) {
        box.setInformativeText(QString::fromUtf8(process.readAllStandardError()));
    }
    box.setStandardButtons(QMessageBox::Close);
    box.exec();
}

void SettingsDialog::onShowServiceRecovery()
{
    const QString text = m_serviceManager ? m_serviceManager->recoveryInstructions()
                                          : HelperProcessManager::recoveryInstructionsText();
    QMessageBox box(this);
    box.setWindowTitle(tr("Helper service recovery"));
    box.setText(text);
    box.setStandardButtons(QMessageBox::Close);
    box.exec();
}

} // namespace zarya
