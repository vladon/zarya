#include "ui/SettingsDialog.h"

#include "helperclient/HelperProcessManager.h"
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
#include <QJsonObject>
#include <QMessageBox>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

SettingsDialog::SettingsDialog(RoutingManager& routingManager, DnsManager& dnsManager,
                             HelperProcessManager* helperManager, QWidget* parent)
    : QDialog(parent)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
    , m_helperManager(helperManager)
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

    m_autostartManager = AutostartManagerFactory::create();
    m_autostartBackendLabel =
        new QLabel(m_autostartManager ? m_autostartManager->backendName() : QString(), this);

    m_startAtLoginCheck = new QCheckBox(QStringLiteral("Start Zarya when I log in"), this);
    const bool osAutostartEnabled =
        m_autostartManager && m_autostartManager->isSupported()
        && m_autostartManager->isEnabled();
    m_startAtLoginCheck->setChecked(settings.startAtLogin() && osAutostartEnabled);
    m_startAtLoginCheck->setEnabled(m_autostartManager && m_autostartManager->isSupported());

    m_startMinimizedToTrayCheck =
        new QCheckBox(QStringLiteral("Start minimized to tray"), this);
    m_startMinimizedToTrayCheck->setChecked(settings.startMinimizedToTray());

    m_autoStartLastProfileCheck =
        new QCheckBox(QStringLiteral("Auto-start last used profile"), this);
    m_autoStartLastProfileCheck->setChecked(settings.autoStartLastProfile());

    m_autoEnableProxyAfterAutoStartCheck = new QCheckBox(
        QStringLiteral("Enable system proxy after auto-starting profile"), this);
    m_autoEnableProxyAfterAutoStartCheck->setChecked(
        settings.autoEnableSystemProxyAfterAutoStart());

    m_autoStartDelaySpin = new QSpinBox(this);
    m_autoStartDelaySpin->setRange(0, 120);
    m_autoStartDelaySpin->setSuffix(QStringLiteral(" s"));
    m_autoStartDelaySpin->setValue(settings.autoStartDelaySeconds());

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

    m_dnsProfileCombo = new QComboBox(this);
    refreshDnsCombo();
    auto* manageDnsButton = new QPushButton(QStringLiteral("Manage DNS Profiles…"), this);
    connect(manageDnsButton, &QPushButton::clicked, this, &SettingsDialog::onManageDnsProfiles);

    auto* dnsRow = new QHBoxLayout;
    dnsRow->addWidget(m_dnsProfileCombo, 1);
    dnsRow->addWidget(manageDnsButton);

    auto* dnsForm = new QFormLayout;
    dnsForm->addRow(QStringLiteral("Active DNS profile"), dnsRow);

    auto* dnsGroup = new QGroupBox(QStringLiteral("DNS"), this);
    dnsGroup->setLayout(dnsForm);

    auto* startupForm = new QFormLayout;
    startupForm->addRow(QStringLiteral("Autostart backend"), m_autostartBackendLabel);
    startupForm->addRow(QString(), m_startAtLoginCheck);
    startupForm->addRow(QString(), m_startMinimizedToTrayCheck);
    startupForm->addRow(QString(), m_autoStartLastProfileCheck);
    startupForm->addRow(QString(), m_autoEnableProxyAfterAutoStartCheck);
    startupForm->addRow(QStringLiteral("Auto-start delay"), m_autoStartDelaySpin);
    if (m_autostartManager && !m_autostartManager->limitations().isEmpty()) {
        auto* autostartLimits = new QLabel(m_autostartManager->limitations(), this);
        autostartLimits->setWordWrap(true);
        startupForm->addRow(QStringLiteral("Autostart notes"), autostartLimits);
    }

    auto* startupGroup = new QGroupBox(QStringLiteral("Startup"), this);
    startupGroup->setLayout(startupForm);

    m_enableExperimentalTunCheck =
        new QCheckBox(QStringLiteral("Enable experimental TUN mode"), this);
    m_enableExperimentalTunCheck->setChecked(settings.enableExperimentalTun());

    m_systemProxyRuntimeRadio =
        new QRadioButton(QStringLiteral("System proxy via Xray"), this);
    m_tunRuntimeRadio =
        new QRadioButton(QStringLiteral("TUN via sing-box (experimental)"), this);
    if (settings.runtimeMode() == RuntimeMode::TunSingBoxExperimental) {
        m_tunRuntimeRadio->setChecked(true);
    } else {
        m_systemProxyRuntimeRadio->setChecked(true);
    }

    m_singBoxPathEdit = new QLineEdit(settings.singBoxExecutablePath(), this);
    auto* browseSingBoxButton = new QPushButton(QStringLiteral("Browse…"), this);
    connect(browseSingBoxButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseSingBox);
    auto* singBoxRow = new QHBoxLayout;
    singBoxRow->addWidget(m_singBoxPathEdit);
    singBoxRow->addWidget(browseSingBoxButton);

    m_tunUseActiveRoutingCheck =
        new QCheckBox(QStringLiteral("Use active RoutingProfile for TUN"), this);
    m_tunUseActiveRoutingCheck->setChecked(settings.tunUseActiveRoutingProfile());

    m_tunUseActiveDnsCheck =
        new QCheckBox(QStringLiteral("Use active DnsProfile for TUN"), this);
    m_tunUseActiveDnsCheck->setChecked(settings.tunUseActiveDnsProfile());

    m_tunEnableDnsHijackCheck =
        new QCheckBox(QStringLiteral("Enable DNS hijack in TUN mode"), this);
    m_tunEnableDnsHijackCheck->setChecked(settings.tunEnableDnsHijack());

    m_tunDnsHijackModeCombo = new QComboBox(this);
    m_tunDnsHijackModeCombo->addItem(QStringLiteral("Hijack to sing-box DNS"),
                                     static_cast<int>(TunDnsHijackMode::HijackToSingBoxDns));
    m_tunDnsHijackModeCombo->addItem(QStringLiteral("Disabled"),
                                     static_cast<int>(TunDnsHijackMode::Disabled));
    const int hijackIndex = m_tunDnsHijackModeCombo->findData(
        static_cast<int>(settings.tunDnsHijackMode()));
    if (hijackIndex >= 0) {
        m_tunDnsHijackModeCombo->setCurrentIndex(hijackIndex);
    }

    m_tunDirectGuiRadio =
        new QRadioButton(QStringLiteral("Run sing-box directly from GUI"), this);
    m_tunHelperRadio =
        new QRadioButton(QStringLiteral("Use zarya-helper experimental"), this);
    if (settings.tunPrivilegeMode() == TunPrivilegeMode::HelperExperimental) {
        m_tunHelperRadio->setChecked(true);
    } else {
        m_tunDirectGuiRadio->setChecked(true);
    }

    m_helperStatusLabel = new QLabel(m_helperManager ? m_helperManager->statusText()
                                                     : QStringLiteral("Helper unavailable"),
                                     this);
    m_startHelperButton = new QPushButton(QStringLiteral("Start Helper"), this);
    m_connectHelperButton = new QPushButton(QStringLiteral("Connect"), this);
    m_checkHelperStatusButton = new QPushButton(QStringLiteral("Check Status"), this);
    connect(m_startHelperButton, &QPushButton::clicked, this, &SettingsDialog::onStartHelper);
    connect(m_connectHelperButton, &QPushButton::clicked, this, &SettingsDialog::onConnectHelper);
    connect(m_checkHelperStatusButton, &QPushButton::clicked, this,
            &SettingsDialog::onCheckHelperStatus);
    if (m_helperManager) {
        connect(m_helperManager, &HelperProcessManager::connectionStateChanged, this,
                [this]() { m_helperStatusLabel->setText(m_helperManager->statusText()); });
    }

    auto* helperButtonsRow = new QHBoxLayout;
    helperButtonsRow->addWidget(m_startHelperButton);
    helperButtonsRow->addWidget(m_connectHelperButton);
    helperButtonsRow->addWidget(m_checkHelperStatusButton);

    auto* tunWarnings = new QLabel(
        QStringLiteral(
            "TUN mode changes system routes and may require administrator/root permissions. "
            "zarya-helper is experimental and is not installed as a privileged service in this "
            "milestone."),
        this);
    tunWarnings->setWordWrap(true);

    auto* experimentalForm = new QFormLayout;
    experimentalForm->addRow(QString(), m_enableExperimentalTunCheck);
    experimentalForm->addRow(QString(), m_systemProxyRuntimeRadio);
    experimentalForm->addRow(QString(), m_tunRuntimeRadio);
    experimentalForm->addRow(QStringLiteral("sing-box executable"), singBoxRow);
    experimentalForm->addRow(QString(), m_tunUseActiveRoutingCheck);
    experimentalForm->addRow(QString(), m_tunUseActiveDnsCheck);
    experimentalForm->addRow(QString(), m_tunEnableDnsHijackCheck);
    experimentalForm->addRow(QStringLiteral("TUN DNS hijack mode"), m_tunDnsHijackModeCombo);
    experimentalForm->addRow(QStringLiteral("TUN privilege mode"), m_tunDirectGuiRadio);
    experimentalForm->addRow(QString(), m_tunHelperRadio);
    experimentalForm->addRow(QStringLiteral("Helper status"), m_helperStatusLabel);
    experimentalForm->addRow(QString(), helperButtonsRow);

    m_tunRequireLocalRuleSetsCheck =
        new QCheckBox(QStringLiteral("Require local .srs rule sets before starting TUN"), this);
    m_tunRequireLocalRuleSetsCheck->setChecked(settings.tunRequireLocalRuleSets());

    m_ruleSetDirLabel = new QLabel(AppPaths::singBoxRuleSetDir(), this);
    m_ruleSetDirLabel->setWordWrap(true);
    m_ruleSetDirLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* ruleSetNote = new QLabel(
        QStringLiteral("Manage rule sets from Tools → sing-box Rule Sets. Xray geoip.dat/geosite.dat "
                       "are separate from sing-box .srs files."),
        this);
    ruleSetNote->setWordWrap(true);

    experimentalForm->addRow(QStringLiteral("Rule sets"), m_tunRequireLocalRuleSetsCheck);
    experimentalForm->addRow(QStringLiteral("Rule-set directory"), m_ruleSetDirLabel);
    experimentalForm->addRow(QString(), ruleSetNote);
    experimentalForm->addRow(QString(), tunWarnings);

    auto* experimentalGroup = new QGroupBox(QStringLiteral("Experimental"), this);
    experimentalGroup->setLayout(experimentalForm);

    m_enableKillSwitchCheck =
        new QCheckBox(QStringLiteral("Enable experimental kill switch"), this);
    m_enableKillSwitchCheck->setChecked(settings.enableExperimentalKillSwitch());

    m_killSwitchModeLabel =
        new QLabel(QStringLiteral("Mode: TUN only experimental"), this);

    m_killSwitchAllowLanCheck =
        new QCheckBox(QStringLiteral("Allow LAN/private networks"), this);
    m_killSwitchAllowLanCheck->setChecked(settings.killSwitchAllowLan());

    m_killSwitchAllowLoopbackCheck =
        new QCheckBox(QStringLiteral("Allow loopback"), this);
    m_killSwitchAllowLoopbackCheck->setChecked(settings.killSwitchAllowLoopback());

    m_killSwitchAllowProxyCheck =
        new QCheckBox(QStringLiteral("Allow traffic to selected proxy server"), this);
    m_killSwitchAllowProxyCheck->setChecked(true);
    m_killSwitchAllowProxyCheck->setEnabled(false);

    m_killSwitchAutoDisableOnStopCheck =
        new QCheckBox(QStringLiteral("Disable kill switch on clean Stop"), this);
    m_killSwitchAutoDisableOnStopCheck->setChecked(settings.killSwitchAutoDisableOnCleanStop());

    m_killSwitchKeepActiveAfterStopCheck =
        new QCheckBox(QStringLiteral("Keep kill switch active after Stop"), this);
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
        QStringLiteral("Kill switch backend: nftables PoC (table inet zarya)"), this);
#elif defined(Q_OS_WIN)
    m_killSwitchBackendLabel = new QLabel(
        QStringLiteral(
            "Kill switch backend: design stub — production should use Windows Filtering "
            "Platform (WFP)."),
        this);
#elif defined(Q_OS_MACOS)
    m_killSwitchBackendLabel = new QLabel(
        QStringLiteral(
            "Kill switch backend: unsupported in 0.16 — PF is not a stable public API for "
            "third-party apps."),
        this);
#else
    m_killSwitchBackendLabel = new QLabel(QStringLiteral("Kill switch backend: unsupported"),
                                          this);
#endif
    m_killSwitchBackendLabel->setWordWrap(true);

    m_killSwitchWarningLabel = new QLabel(
        QStringLiteral(
            "Experimental kill switch changes firewall/routing rules. A bug may block network "
            "access until rules are removed manually. Requires zarya-helper mode. Use only if you "
            "understand the recovery procedure."),
        this);
    m_killSwitchWarningLabel->setWordWrap(true);

    m_testKillSwitchButton = new QPushButton(QStringLiteral("Test Support"), this);
    m_enableKillSwitchButton = new QPushButton(QStringLiteral("Enable Now"), this);
    m_disableKillSwitchButton = new QPushButton(QStringLiteral("Disable Now"), this);
    m_killSwitchRecoveryButton =
        new QPushButton(QStringLiteral("Show Recovery Instructions"), this);
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

    auto* killSwitchGroup = new QGroupBox(QStringLiteral("Kill Switch (experimental)"), this);
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
        m_startHelperButton->setEnabled(helperUi);
        m_connectHelperButton->setEnabled(helperUi);
        m_checkHelperStatusButton->setEnabled(helperUi);
        updateKillSwitchControls();
    };
    connect(m_tunEnableDnsHijackCheck, &QCheckBox::toggled, this, updateRuntimeControls);
    updateRuntimeControls();
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
    layout->addWidget(coreGroup);
    layout->addWidget(proxyGroup);
    layout->addWidget(testingGroup);
    layout->addWidget(routingGroup);
    layout->addWidget(dnsGroup);
    layout->addWidget(startupGroup);
    layout->addWidget(desktopGroup);
    layout->addWidget(experimentalGroup);
    layout->addWidget(killSwitchGroup);
    layout->addWidget(buttons);
    resize(620, 1080);
}

void SettingsDialog::onBrowseSingBox()
{
    const QString path =
        QFileDialog::getOpenFileName(this, QStringLiteral("Select sing-box executable"), {},
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
    box.setWindowTitle(QStringLiteral("Experimental TUN mode"));
    box.setText(QStringLiteral(
        "TUN mode is experimental. It may change network routes and DNS behavior.\n\n"
        "If it fails, Zarya will attempt to stop sing-box and restore state, but this mode is "
        "not production-ready yet.\n\n"
        "Kill switch is experimental and requires zarya-helper mode."));
    QPushButton* enableButton = box.addButton(QStringLiteral("Enable Experimental TUN"),
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
                QMessageBox::warning(this, QStringLiteral("Autostart"), autostartError);
                return false;
            }
        } else if (m_autostartManager->isEnabled()
                   && !m_autostartManager->disable(&autostartError)) {
            QMessageBox::warning(this, QStringLiteral("Autostart"), autostartError);
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
        QMessageBox::warning(this, QStringLiteral("Kill switch"), error);
        return;
    }
    QJsonObject payload;
    if (!m_helperManager->killSwitchCheckSupport(&payload, &error)) {
        QMessageBox::warning(this, QStringLiteral("Kill switch"), error);
        return;
    }
    QMessageBox::information(
        this, QStringLiteral("Kill switch support"),
        QStringLiteral("Backend: %1\nPrivileged: %2\nSupported: %3\n\n%4")
            .arg(payload.value(QStringLiteral("backend")).toString(),
                 payload.value(QStringLiteral("privileged")).toBool() ? QStringLiteral("yes")
                                                                      : QStringLiteral("no"),
                 payload.value(QStringLiteral("supported")).toBool() ? QStringLiteral("yes")
                                                                     : QStringLiteral("no"),
                 payload.value(QStringLiteral("lastError")).toString()));
}

void SettingsDialog::onEnableKillSwitchNow()
{
    QMessageBox::information(
        this, QStringLiteral("Kill switch"),
        QStringLiteral(
            "Kill switch is enabled automatically when you start TUN mode with kill switch "
            "enabled.\n\nSelect a profile and press Start, or use a running TUN session."));
}

void SettingsDialog::onDisableKillSwitchNow()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->connectToHelper(&error)) {
        QMessageBox::warning(this, QStringLiteral("Kill switch"), error);
        return;
    }
    if (!m_helperManager->killSwitchDisable(&error)) {
        QMessageBox::warning(this, QStringLiteral("Kill switch"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Kill switch"),
                             QStringLiteral("Kill switch disabled."));
}

void SettingsDialog::onShowKillSwitchRecovery()
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Kill switch recovery"));
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
        QMessageBox::warning(this, QStringLiteral("Helper"), error);
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
        QMessageBox::warning(this, QStringLiteral("Helper"), error);
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
        QMessageBox::warning(this, QStringLiteral("Helper"), error);
        return;
    }
    m_helperStatusLabel->setText(
        QStringLiteral("running=%1, pid=%2")
            .arg(payload.value(QStringLiteral("running")).toBool() ? QStringLiteral("yes")
                                                                   : QStringLiteral("no"))
            .arg(payload.value(QStringLiteral("pid")).toInt()));
}

} // namespace zarya
