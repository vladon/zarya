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
#include "features/FeatureGate.h"
#include "features/FeaturePolicy.h"
#include "packaging/InstallationMode.h"
#include "packaging/WindowsInstallInfo.h"
#include "storage/DefaultSettings.h"
#include "storage/AppSettings.h"
#include "ui/DnsManagerDialog.h"
#include "ui/RoutingManagerDialog.h"
#include "ui/theme/ThemeManager.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"
#include "ui/theme/ThemeMode.h"

#include "base/algorithm.h"
#include "base/basic_types.h"
#include "base/object_ptr.h"
#include <rpl/rpl.h>
#include "styles/style_layers.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/widgets/scroll_area.h"

#if defined(Q_OS_LINUX)
#include "platform/linux/LinuxSystemProxyManager.h"
#endif

#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QScreen>
#include <QSizePolicy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

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

    m_languageCombo = new ZaryaSelector(this);
    QVector<ZaryaSelectorItem> languages;
    for (const LanguageInfo& lang : LanguageManager::instance().availableLanguages()) {
        languages.push_back({lang.code, lang.nativeName, true});
    }
    m_languageCombo->setItems(
        std::move(languages), LanguageManager::instance().currentLanguageCode());

    m_themeCombo = new ZaryaSelector(this);
    m_themeCombo->setItems(
        {
            {themeModeToString(ThemeMode::System), tr("System"), true},
            {themeModeToString(ThemeMode::Light), tr("Light"), true},
            {themeModeToString(ThemeMode::Dark), tr("Dark"), true},
        },
        themeModeToString(ThemeManager::instance().mode()));
    connect(m_themeCombo, &ZaryaSelector::currentKeyChanged, this, [this](const QString& mode) {
        ThemeManager::instance().setMode(themeModeFromString(mode));
    });

    auto* generalGroup = new ZaryaFormSection(tr("General"), this);
    generalGroup->addWidget(new ZaryaFormRow(tr("Language"), m_languageCombo, generalGroup));
    generalGroup->addWidget(new ZaryaFormRow(tr("Theme"), m_themeCombo, generalGroup));

    m_mixedPortSpin = new ZaryaNumberField(QStringLiteral("1–65535"), 1, 65535, this);
    m_mixedPortSpin->setValue(settings.mixedPort());
    connect(m_mixedPortSpin, &ZaryaNumberField::valueChanged, this,
            &SettingsDialog::updateProxyEndpointLabel);

    m_proxyEndpointLabel = new ZaryaBodyText({}, this);
    updateProxyEndpointLabel();

    m_autoEnableSystemProxyCheck = new ZaryaCheckBox(
        tr("Enable system proxy when profile starts"),
        this,
        settings.autoEnableSystemProxyOnStart());

    m_restoreProxyOnExitCheck = new ZaryaCheckBox(
        tr("Restore previous proxy settings on stop/exit"),
        this,
        settings.restoreProxyOnExit());

    const std::unique_ptr<ISystemProxyManager> proxyManager = SystemProxyManagerFactory::create();
    m_proxyBackendLabel = new ZaryaBodyText(
        proxyManager ? proxyManager->backendName() : QString(), this);
    m_proxySupportLabel = new ZaryaBodyText(
        proxyManager && proxyManager->isSupported() ? tr("Full") : tr("Unavailable"), this);
    m_proxyLimitationsLabel = new ZaryaBodyText(
        proxyManager ? proxyManager->limitations() : QString(), this);

    m_linuxDesktopLabel = new ZaryaBodyText({}, this);
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

    m_macApplyAllServicesCheck = new ZaryaCheckBox(
        tr("Apply proxy to all network services"),
        this,
        settings.macApplyProxyToAllServices());
    m_macPreferredServiceEdit = new ZaryaTextField(tr("e.g. Wi-Fi (optional)"), this);
    m_macPreferredServiceEdit->setText(settings.macPreferredNetworkService());
#if !defined(Q_OS_MACOS)
    m_macApplyAllServicesCheck->hide();
    m_macPreferredServiceEdit->hide();
#endif

    m_testUrlEdit = new ZaryaTextField(tr("Test URL"), this);
    auto* testUrlPreset = new ZaryaSelector(this);
    QVector<ZaryaSelectorItem> testUrlItems;
    for (const QString& preset : DefaultSettings::testUrlPresets()) {
        testUrlItems.push_back({preset, preset, true});
    }
    const QString currentTestUrl = settings.testUrl().trimmed();
    const QString effectiveTestUrl = currentTestUrl.isEmpty()
        ? DefaultSettings::testUrl()
        : currentTestUrl;
    if (!DefaultSettings::testUrlPresets().contains(effectiveTestUrl)) {
        testUrlItems.push_front({effectiveTestUrl, effectiveTestUrl, true});
    }
    m_testUrlEdit->setText(effectiveTestUrl);
    testUrlPreset->setItems(std::move(testUrlItems), effectiveTestUrl);
    connect(testUrlPreset, &ZaryaSelector::currentKeyChanged, m_testUrlEdit,
            &ZaryaTextField::setText);

    m_tcpTimeoutSpin = new ZaryaNumberField(QStringLiteral("1000–60000"), 1000, 60000, this);
    m_tcpTimeoutSpin->setValue(settings.tcpTestTimeoutMs());

    m_realDelayTimeoutSpin = new ZaryaNumberField(
        QStringLiteral("1000–60000"), 1000, 60000, this);
    m_realDelayTimeoutSpin->setValue(settings.realDelayTimeoutMs());

    m_maxConcurrentTestsSpin = new ZaryaNumberField(QStringLiteral("1–10"), 1, 10, this);
    m_maxConcurrentTestsSpin->setValue(settings.maxConcurrentTests());

    m_skipTcpBeforeRealDelayCheck = new ZaryaCheckBox(
        tr("Skip TCP test before real delay"), this, settings.skipTcpBeforeRealDelay());

    m_minimizeToTrayOnCloseCheck = new ZaryaCheckBox(
        tr("Close button hides to tray"), this, settings.minimizeToTrayOnClose());
    m_minimizeToTrayOnMinimizeCheck = new ZaryaCheckBox(
        tr("Minimize hides to tray"), this, settings.minimizeToTrayOnMinimize());
    m_showTrayNotificationsCheck = new ZaryaCheckBox(
        tr("Show tray notifications"), this, settings.showTrayNotifications());
    m_confirmExitWhileRunningCheck = new ZaryaCheckBox(
        tr("Confirm exit while core is running"), this, settings.confirmExitWhileRunning());

    m_autostartManager = AutostartManagerFactory::create();
    m_autostartBackendLabel = new ZaryaBodyText(
        m_autostartManager ? m_autostartManager->backendName() : QString(), this);

    const bool osAutostartEnabled =
        m_autostartManager && m_autostartManager->isSupported()
        && m_autostartManager->isEnabled();
    m_startAtLoginCheck = new ZaryaCheckBox(
        tr("Start Zarya when I log in"),
        this,
        settings.startAtLogin() && osAutostartEnabled);
    m_startAtLoginCheck->setEnabled(m_autostartManager && m_autostartManager->isSupported());

    m_startMinimizedToTrayCheck = new ZaryaCheckBox(
        tr("Start minimized to tray"), this, settings.startMinimizedToTray());
    m_autoStartLastProfileCheck = new ZaryaCheckBox(
        tr("Auto-start last used profile"), this, settings.autoStartLastProfile());
    m_autoEnableProxyAfterAutoStartCheck = new ZaryaCheckBox(
        tr("Enable system proxy after auto-starting profile"),
        this,
        settings.autoEnableSystemProxyAfterAutoStart());

    m_autoStartDelaySpin = new ZaryaNumberField(QStringLiteral("0–120"), 0, 120, this);
    m_autoStartDelaySpin->setValue(settings.autoStartDelaySeconds());

    auto* coreGroup = new ZaryaFormSection(tr("Cores"), this);
    coreGroup->addWidget(new ZaryaFormRow(tr("Local proxy port"), m_mixedPortSpin, this));

    auto* proxyGroup = new ZaryaFormSection(tr("Proxy Mode"), this);
    proxyGroup->addWidget(new ZaryaFormRow(tr("Backend"), m_proxyBackendLabel, this));
    proxyGroup->addWidget(new ZaryaFormRow(tr("Support level"), m_proxySupportLabel, this));
    proxyGroup->addWidget(new ZaryaFormRow(tr("Limitations"), m_proxyLimitationsLabel, this));
    proxyGroup->addWidget(
        new ZaryaFormRow(tr("System proxy endpoint"), m_proxyEndpointLabel, this));
#if defined(Q_OS_LINUX)
    proxyGroup->addWidget(new ZaryaFormRow(tr("Desktop"), m_linuxDesktopLabel, this));
#endif
    proxyGroup->addWidget(m_autoEnableSystemProxyCheck);
    proxyGroup->addWidget(m_restoreProxyOnExitCheck);
#if defined(Q_OS_MACOS)
    proxyGroup->addWidget(m_macApplyAllServicesCheck);
    proxyGroup->addWidget(
        new ZaryaFormRow(tr("Preferred network service"), m_macPreferredServiceEdit, this));
#endif

    auto* testingGroup = new ZaryaFormSection(tr("Testing"), this);
    testingGroup->addWidget(new ZaryaFormRow(tr("Test URL"), m_testUrlEdit, testingGroup));
    testingGroup->addWidget(new ZaryaFormRow(tr("Test URL preset"), testUrlPreset, this));
    testingGroup->addWidget(new ZaryaFormRow(tr("TCP timeout (ms)"), m_tcpTimeoutSpin, this));
    testingGroup->addWidget(
        new ZaryaFormRow(tr("Real delay timeout (ms)"), m_realDelayTimeoutSpin, this));
    testingGroup->addWidget(
        new ZaryaFormRow(tr("Max concurrent tests"), m_maxConcurrentTestsSpin, this));
    testingGroup->addWidget(m_skipTcpBeforeRealDelayCheck);

    auto* desktopGroup = new ZaryaFormSection(tr("Desktop behavior"), this);
    desktopGroup->addWidget(m_minimizeToTrayOnCloseCheck);
    desktopGroup->addWidget(m_minimizeToTrayOnMinimizeCheck);
    desktopGroup->addWidget(m_showTrayNotificationsCheck);
    desktopGroup->addWidget(m_confirmExitWhileRunningCheck);

    m_routingProfileCombo = new ZaryaSelector(this);
    refreshRoutingCombo();
    auto* manageRoutingButton = new ZaryaActionButton(tr("Manage Routing Profiles…"), this);
    connect(manageRoutingButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onManageRoutingProfiles);

    auto* routingRow = new QWidget(this);
    auto* routingRowLayout = new QHBoxLayout(routingRow);
    routingRowLayout->setContentsMargins(0, 0, 0, 0);
    routingRowLayout->setSpacing(8);
    routingRowLayout->addWidget(m_routingProfileCombo, 1);
    routingRowLayout->addWidget(manageRoutingButton);

    auto* routingGroup = new ZaryaFormSection(tr("Routing"), this);
    routingGroup->addWidget(
        new ZaryaFormRow(tr("Active routing profile"), routingRow, routingGroup));

    m_dnsProfileCombo = new ZaryaSelector(this);
    refreshDnsCombo();
    auto* manageDnsButton = new ZaryaActionButton(tr("Manage DNS Profiles…"), this);
    connect(manageDnsButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onManageDnsProfiles);

    auto* dnsRow = new QWidget(this);
    auto* dnsRowLayout = new QHBoxLayout(dnsRow);
    dnsRowLayout->setContentsMargins(0, 0, 0, 0);
    dnsRowLayout->setSpacing(8);
    dnsRowLayout->addWidget(m_dnsProfileCombo, 1);
    dnsRowLayout->addWidget(manageDnsButton);

    auto* dnsGroup = new ZaryaFormSection(tr("DNS"), this);
    dnsGroup->addWidget(new ZaryaFormRow(tr("Active DNS profile"), dnsRow, dnsGroup));

    auto* startupGroup = new ZaryaFormSection(tr("Startup"), this);
    startupGroup->addWidget(
        new ZaryaFormRow(tr("Autostart backend"), m_autostartBackendLabel, startupGroup));
    startupGroup->addWidget(m_startAtLoginCheck);
    startupGroup->addWidget(m_startMinimizedToTrayCheck);
    startupGroup->addWidget(m_autoStartLastProfileCheck);
    startupGroup->addWidget(m_autoEnableProxyAfterAutoStartCheck);
    startupGroup->addWidget(
        new ZaryaFormRow(tr("Auto-start delay (seconds)"), m_autoStartDelaySpin, startupGroup));
    if (m_autostartManager && !m_autostartManager->limitations().isEmpty()) {
        auto* autostartLimits = new ZaryaBodyText(m_autostartManager->limitations(), this);
        startupGroup->addWidget(
            new ZaryaFormRow(tr("Autostart notes"), autostartLimits, startupGroup));
    }

    m_experimentalGroup = new ZaryaFormSection(
        tr("Experimental (TUN · helper · kill switch)"), this);
    m_enableExperimentalTunCheck = new ZaryaCheckBox(
        tr("Enable experimental TUN mode"), this, settings.enableExperimentalTun());

    m_runtimeModeGroup = new ZaryaRadioGroup(
        static_cast<int>(settings.runtimeMode()), this);
    m_runtimeModeGroup->addOption(
        static_cast<int>(RuntimeMode::SystemProxyXray), tr("System proxy via Xray"));
    m_runtimeModeGroup->addOption(
        static_cast<int>(RuntimeMode::TunSingBoxExperimental),
        tr("TUN via sing-box (experimental)"));

    m_tunUseActiveRoutingCheck = new ZaryaCheckBox(
        tr("Use active RoutingProfile for TUN"), this,
        settings.tunUseActiveRoutingProfile());
    m_tunUseActiveDnsCheck = new ZaryaCheckBox(
        tr("Use active DnsProfile for TUN"), this,
        settings.tunUseActiveDnsProfile());
    m_tunEnableDnsHijackCheck = new ZaryaCheckBox(
        tr("Enable DNS hijack in TUN mode"), this, settings.tunEnableDnsHijack());

    const auto enumKey = [](int value) { return QString::number(value); };
    m_tunDnsHijackModeCombo = new ZaryaSelector(this);
    m_tunDnsHijackModeCombo->setItems({
        {enumKey(static_cast<int>(TunDnsHijackMode::HijackToSingBoxDns)),
         tr("Hijack to sing-box DNS")},
        {enumKey(static_cast<int>(TunDnsHijackMode::Disabled)), tr("Disabled")},
    }, enumKey(static_cast<int>(settings.tunDnsHijackMode())));

    m_tunPrivilegeModeGroup = new ZaryaRadioGroup(
        static_cast<int>(settings.tunPrivilegeMode()), this);
    m_tunPrivilegeModeGroup->addOption(
        static_cast<int>(TunPrivilegeMode::DirectFromGui),
        tr("Run sing-box directly from GUI"));
    m_tunPrivilegeModeGroup->addOption(
        static_cast<int>(TunPrivilegeMode::HelperExperimental),
        tr("Use zarya-helper experimental"));

    m_helperBackendLabel = new ZaryaBodyText({}, this);
    m_helperServiceStatusLabel = new ZaryaBodyText({}, this);
    m_helperStatusLabel = new ZaryaBodyText(
        m_helperManager ? m_helperManager->statusText() : tr("Helper unavailable"), this);
    m_installServiceButton = new ZaryaActionButton(tr("Install"), this);
    m_uninstallServiceButton = new ZaryaActionButton(tr("Uninstall"), this);
    m_startServiceButton = new ZaryaActionButton(tr("Start Service"), this);
    m_stopServiceButton = new ZaryaActionButton(tr("Stop Service"), this);
    m_restartServiceButton = new ZaryaActionButton(tr("Restart Service"), this);
    m_startHelperButton = new ZaryaActionButton(tr("Start Manual Helper"), this);
    m_connectHelperButton = new ZaryaActionButton(tr("Connect"), this);
    m_checkHelperStatusButton = new ZaryaActionButton(tr("Check Status"), this);
    m_serviceSelfTestButton = new ZaryaActionButton(tr("Run Self-Test"), this);
    m_serviceRecoveryButton = new ZaryaActionButton(tr("Show Recovery Instructions"), this);
    m_recoverKillSwitchOnUninstallCheck = new ZaryaCheckBox(
        tr("Also recover/remove Zarya kill switch rules on uninstall"), this);

    connect(m_installServiceButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onInstallService);
    connect(m_uninstallServiceButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onUninstallService);
    connect(m_startServiceButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onStartService);
    connect(m_stopServiceButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onStopService);
    connect(m_restartServiceButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onRestartService);
    connect(m_startHelperButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onStartHelper);
    connect(m_connectHelperButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onConnectHelper);
    connect(m_checkHelperStatusButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onCheckHelperStatus);
    connect(m_serviceSelfTestButton, &ZaryaActionButton::clicked,
            this, &SettingsDialog::onServiceSelfTest);
    connect(m_serviceRecoveryButton, &ZaryaActionButton::clicked, this,
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

    auto* serviceButtons = new QWidget(this);
    auto* serviceButtonsRow = new QHBoxLayout(serviceButtons);
    serviceButtonsRow->setContentsMargins(0, 0, 0, 0);
    serviceButtonsRow->addWidget(m_installServiceButton);
    serviceButtonsRow->addWidget(m_uninstallServiceButton);
    serviceButtonsRow->addWidget(m_startServiceButton);
    serviceButtonsRow->addWidget(m_stopServiceButton);
    serviceButtonsRow->addWidget(m_restartServiceButton);

    auto* helperButtons = new QWidget(this);
    auto* helperButtonsRow = new QHBoxLayout(helperButtons);
    helperButtonsRow->setContentsMargins(0, 0, 0, 0);
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
    m_helperServiceWarningLabel = new ZaryaBodyText(helperWarningText, this);

    auto* tunWarnings = new ZaryaBodyText(
        tr("TUN mode requires sing-box and may require zarya-helper. System-proxy mode does not "
           "require the helper service."),
        this);

    m_tunRequireLocalRuleSetsCheck = new ZaryaCheckBox(
        tr("Require local .srs rule sets before starting TUN"), this,
        settings.tunRequireLocalRuleSets());

    m_ruleSetDirLabel = new ZaryaBodyText(AppPaths::singBoxRuleSetDir(), this);

    auto* ruleSetNote = new ZaryaBodyText(
        tr("Manage rule sets from Tools → sing-box Rule Sets. Xray geoip.dat/geosite.dat "
           "are separate from sing-box .srs files."),
        this);

    m_experimentalGroup->addWidget(m_enableExperimentalTunCheck);
    m_experimentalGroup->addWidget(
        new ZaryaFormRow(tr("Runtime mode"), m_runtimeModeGroup, m_experimentalGroup));
    m_experimentalGroup->addWidget(m_tunUseActiveRoutingCheck);
    m_experimentalGroup->addWidget(m_tunUseActiveDnsCheck);
    m_experimentalGroup->addWidget(m_tunEnableDnsHijackCheck);
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("TUN DNS hijack mode"), m_tunDnsHijackModeCombo, m_experimentalGroup));
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("TUN privilege mode"), m_tunPrivilegeModeGroup, m_experimentalGroup));
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("Privileged helper backend"), m_helperBackendLabel, m_experimentalGroup));
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("Service status"), m_helperServiceStatusLabel, m_experimentalGroup));
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("IPC connection"), m_helperStatusLabel, m_experimentalGroup));
    m_experimentalGroup->addWidget(serviceButtons);
    m_experimentalGroup->addWidget(m_recoverKillSwitchOnUninstallCheck);
    m_experimentalGroup->addWidget(helperButtons);
    m_experimentalGroup->addWidget(m_helperServiceWarningLabel);
    m_experimentalGroup->addWidget(m_tunRequireLocalRuleSetsCheck);
    m_experimentalGroup->addWidget(new ZaryaFormRow(
        tr("Rule-set directory"), m_ruleSetDirLabel, m_experimentalGroup));
    m_experimentalGroup->addWidget(ruleSetNote);
    m_experimentalGroup->addWidget(tunWarnings);

    m_releaseChannelCombo = new ZaryaSelector(this);
    m_releaseChannelCombo->setItems(
        {
            {QStringLiteral("dev"), tr("Dev"), true},
            {QStringLiteral("beta"), tr("Beta"), true},
            {QStringLiteral("rc"), tr("Release Candidate"), true},
            {QStringLiteral("stable"), tr("Stable"), true},
        },
        settings.releaseChannelKey());

    m_showExperimentalFeaturesCheck = new ZaryaCheckBox(
        tr("Show experimental features (TUN, helper, kill switch)"),
        this,
        settings.showExperimentalFeatures());

    m_experimentalGatePanel = new QWidget(this);
    auto* gateLabel = new ZaryaBodyText(
        tr("Experimental features are hidden in release-candidate and stable builds.\n"
           "Xray system-proxy mode is the recommended stable path."),
        m_experimentalGatePanel);
    m_showExperimentalFeaturesButton =
        new ZaryaActionButton(tr("Show Experimental Features…"), m_experimentalGatePanel);
    connect(m_showExperimentalFeaturesButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onShowExperimentalFeatures);
    auto* gateLayout = new QVBoxLayout(m_experimentalGatePanel);
    gateLayout->setContentsMargins(0, 0, 0, 0);
    gateLayout->addWidget(gateLabel);
    gateLayout->addWidget(m_showExperimentalFeaturesButton);

    auto* releaseGroup = new ZaryaFormSection(tr("Release channel"), this);
    releaseGroup->addWidget(
        new ZaryaFormRow(tr("Release channel"), m_releaseChannelCombo, releaseGroup));
    releaseGroup->addWidget(m_showExperimentalFeaturesCheck);
    releaseGroup->addWidget(m_experimentalGatePanel);

    connect(m_releaseChannelCombo, &ZaryaSelector::currentKeyChanged, this, [this](const QString& channel) {
        const ReleaseChannel releaseChannel =
            FeaturePolicy::releaseChannelFromString(channel);
        if (FeaturePolicy::isStableLikeChannel(releaseChannel)) {
            m_showExperimentalFeaturesCheck->setChecked(false);
        } else {
            m_showExperimentalFeaturesCheck->setChecked(
                FeaturePolicy::defaultShowExperimentalFeatures(releaseChannel));
        }
        updateExperimentalVisibility();
    });
    connect(m_showExperimentalFeaturesCheck, &ZaryaCheckBox::toggled, this,
            &SettingsDialog::updateExperimentalVisibility);

    m_appUpdateChannelCombo = new ZaryaSelector(this);
    m_appUpdateChannelCombo->setItems(
        {
            {QStringLiteral("dev"), tr("Dev"), true},
            {QStringLiteral("beta"), tr("Beta"), true},
            {QStringLiteral("rc"), tr("Release Candidate"), true},
            {QStringLiteral("stable"), tr("Stable"), true},
        },
        settings.appUpdateChannelKey());

    m_checkAppUpdatesOnStartupCheck = new ZaryaCheckBox(
        tr("Check app updates on startup"), this, settings.checkAppUpdatesOnStartup());

    m_appUpdateManifestUrlEdit = new ZaryaTextField(
        tr("Leave empty to use Help → Check for App Updates with a local manifest"), this);
    m_appUpdateManifestUrlEdit->setText(settings.appUpdateManifestUrl());

    m_allowUnsignedAppUpdatesCheck = new ZaryaCheckBox(
        tr("Allow unsigned app update download (no checksum)"),
        this,
        settings.allowUnsignedAppUpdates());

    auto* appUpdatesNote = new ZaryaBodyText(
        tr("App updates include Zarya and its built-in Xray. Core updates below apply to sing-box."),
        this);

    auto* appUpdatesGroup = new ZaryaFormSection(tr("App updates"), this);
    appUpdatesGroup->addWidget(
        new ZaryaFormRow(tr("Channel"), m_appUpdateChannelCombo, appUpdatesGroup));
    appUpdatesGroup->addWidget(m_checkAppUpdatesOnStartupCheck);
    appUpdatesGroup->addWidget(
        new ZaryaFormRow(tr("Manifest URL"), m_appUpdateManifestUrlEdit, appUpdatesGroup));
    appUpdatesGroup->addWidget(m_allowUnsignedAppUpdatesCheck);
    appUpdatesGroup->addWidget(appUpdatesNote);

    m_allowCoreUpdateWithoutChecksumCheck = new ZaryaCheckBox(
        tr("Allow installing core archives without checksum verification"),
        this,
        settings.allowCoreUpdateWithoutChecksum());

    m_allowManageExternalCorePathsCheck = new ZaryaCheckBox(
        tr("Allow managing cores outside Zarya-managed directory"),
        this,
        settings.allowManageExternalCorePaths());

    m_coreBackupRetentionSpin = new ZaryaNumberField(QStringLiteral("1–10"), 1, 10, this);
    m_coreBackupRetentionSpin->setValue(settings.coreBackupRetentionCount());

    m_githubApiTimeoutSpin = new ZaryaNumberField(QStringLiteral("5–120"), 5, 120, this);
    m_githubApiTimeoutSpin->setValue(settings.githubApiTimeoutSeconds());

    m_checkCoreUpdatesOnStartupCheck = new ZaryaCheckBox(
        tr("Check core updates on startup"), this, settings.checkCoreUpdatesOnStartup());

    auto* coreUpdatesGroup = new ZaryaFormSection(tr("Core updates"), this);
    coreUpdatesGroup->addWidget(m_allowCoreUpdateWithoutChecksumCheck);
    coreUpdatesGroup->addWidget(m_allowManageExternalCorePathsCheck);
    coreUpdatesGroup->addWidget(
        new ZaryaFormRow(tr("Backup retention"), m_coreBackupRetentionSpin, coreUpdatesGroup));
    coreUpdatesGroup->addWidget(new ZaryaFormRow(
        tr("GitHub API timeout (seconds)"), m_githubApiTimeoutSpin, coreUpdatesGroup));
    coreUpdatesGroup->addWidget(m_checkCoreUpdatesOnStartupCheck);

    m_killSwitchGroup = new ZaryaFormSection(
        tr("Kill Switch — Experimental · Requires helper · Linux/Windows PoC"), this);
    m_enableKillSwitchCheck = new ZaryaCheckBox(
        tr("Enable experimental kill switch"), this,
        settings.enableExperimentalKillSwitch());

    m_killSwitchModeLabel =
        new ZaryaBodyText(tr("Mode: TUN only experimental"), this);

    m_killSwitchAllowLanCheck = new ZaryaCheckBox(
        tr("Allow LAN/private networks"), this, settings.killSwitchAllowLan());

    m_killSwitchAllowLoopbackCheck = new ZaryaCheckBox(
        tr("Allow loopback"), this, settings.killSwitchAllowLoopback());

    m_killSwitchAllowProxyCheck = new ZaryaCheckBox(
        tr("Allow traffic to selected proxy server"), this, true);
    m_killSwitchAllowProxyCheck->setEnabled(false);

    m_killSwitchAutoDisableOnStopCheck = new ZaryaCheckBox(
        tr("Disable kill switch on clean Stop"), this,
        settings.killSwitchAutoDisableOnCleanStop());

    m_killSwitchKeepActiveAfterStopCheck = new ZaryaCheckBox(
        tr("Keep kill switch active after Stop"), this,
        !settings.killSwitchAutoDisableOnCleanStop());

    connect(m_killSwitchAutoDisableOnStopCheck, &ZaryaCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) {
                    m_killSwitchKeepActiveAfterStopCheck->setChecked(false);
                }
            });
    connect(m_killSwitchKeepActiveAfterStopCheck, &ZaryaCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) {
                    m_killSwitchAutoDisableOnStopCheck->setChecked(false);
                }
            });

#if defined(Q_OS_LINUX)
    m_killSwitchBackendLabel = new ZaryaBodyText(
        tr("Kill switch backend: nftables PoC (table inet zarya)"), this);
#elif defined(Q_OS_WIN)
    m_killSwitchBackendLabel = new ZaryaBodyText(
        tr("Kill switch backend: Windows WFP PoC (requires Administrator helper)"),
        this);
#elif defined(Q_OS_MACOS)
    m_killSwitchBackendLabel = new ZaryaBodyText(
        tr("Kill switch backend: unsupported in 0.16 — PF is not a stable public API for "
           "third-party apps."),
        this);
#else
    m_killSwitchBackendLabel = new ZaryaBodyText(
        tr("Kill switch backend: unsupported"), this);
#endif

    m_killSwitchWarningLabel = new ZaryaBodyText(
        tr("Experimental kill switch changes firewall/routing rules. A bug may block network "
           "access until rules are removed manually. Requires zarya-helper mode. Use only if you "
           "understand the recovery procedure."),
        this);
    m_testKillSwitchButton = new ZaryaActionButton(tr("Test Support"), this);
    m_enableKillSwitchButton = new ZaryaActionButton(tr("How it works"), this);
    m_disableKillSwitchButton = new ZaryaActionButton(tr("Disable Now"), this);
    m_killSwitchRecoveryButton =
        new ZaryaActionButton(tr("Show Recovery Instructions"), this);
    connect(m_testKillSwitchButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onTestKillSwitchSupport);
    connect(m_enableKillSwitchButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onEnableKillSwitchNow);
    connect(m_disableKillSwitchButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onDisableKillSwitchNow);
    connect(m_killSwitchRecoveryButton, &ZaryaActionButton::clicked, this,
            &SettingsDialog::onShowKillSwitchRecovery);

    auto* killSwitchButtons = new QWidget(this);
    auto* killSwitchButtonsRow = new QHBoxLayout(killSwitchButtons);
    killSwitchButtonsRow->setContentsMargins(0, 0, 0, 0);
    killSwitchButtonsRow->addWidget(m_testKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_enableKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_disableKillSwitchButton);
    killSwitchButtonsRow->addWidget(m_killSwitchRecoveryButton);

    m_killSwitchGroup->addWidget(m_enableKillSwitchCheck);
    m_killSwitchGroup->addWidget(m_killSwitchModeLabel);
    m_killSwitchGroup->addWidget(m_killSwitchAllowLanCheck);
    m_killSwitchGroup->addWidget(m_killSwitchAllowLoopbackCheck);
    m_killSwitchGroup->addWidget(m_killSwitchAllowProxyCheck);
    m_killSwitchGroup->addWidget(m_killSwitchAutoDisableOnStopCheck);
    m_killSwitchGroup->addWidget(m_killSwitchKeepActiveAfterStopCheck);
    m_killSwitchGroup->addWidget(m_killSwitchBackendLabel);
    m_killSwitchGroup->addWidget(m_killSwitchWarningLabel);
    m_killSwitchGroup->addWidget(killSwitchButtons);

    const auto updateRuntimeControls = [this]() {
        const bool enabled = m_enableExperimentalTunCheck->isChecked();
        m_runtimeModeGroup->setEnabled(enabled);
        m_tunUseActiveRoutingCheck->setEnabled(enabled);
        m_tunUseActiveDnsCheck->setEnabled(enabled);
        m_tunEnableDnsHijackCheck->setEnabled(enabled);
        m_tunDnsHijackModeCombo->setEnabled(enabled && m_tunEnableDnsHijackCheck->isChecked());
        m_tunPrivilegeModeGroup->setEnabled(enabled);
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
    connect(m_tunEnableDnsHijackCheck, &ZaryaCheckBox::toggled,
            this, updateRuntimeControls);
    updateRuntimeControls();
    refreshHelperServiceUi();
    connect(m_enableExperimentalTunCheck, &ZaryaCheckBox::toggled,
            this, updateRuntimeControls);
    connect(m_enableKillSwitchCheck, &ZaryaCheckBox::toggled, this,
            &SettingsDialog::updateKillSwitchControls);
    connect(m_tunPrivilegeModeGroup, &ZaryaRadioGroup::valueChanged,
            this, &SettingsDialog::updateKillSwitchControls);
    updateKillSwitchControls();

    auto* buttons = new ZaryaDialogActionRow(tr("Save"), tr("Cancel"), this);
    connect(buttons, &ZaryaDialogActionRow::accepted, this, [this]() {
        if (validateAndSave()) {
            accept();
        }
    });
    connect(buttons, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* scroll = Ui::CreateChild<Ui::ScrollArea>(this, st::boxScroll);
    scroll->setWidgetResizable(true);
    auto content = object_ptr<Ui::RpWidget>(scroll);
    auto* contentWidget = content.data();
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(generalGroup);
    contentLayout->addWidget(coreGroup);
    contentLayout->addWidget(proxyGroup);
    contentLayout->addWidget(routingGroup);
    contentLayout->addWidget(dnsGroup);
    contentLayout->addWidget(startupGroup);
    contentLayout->addWidget(desktopGroup);
    contentLayout->addWidget(appUpdatesGroup);
    contentLayout->addWidget(coreUpdatesGroup);
    contentLayout->addWidget(testingGroup);
    contentLayout->addWidget(releaseGroup);
    contentLayout->addWidget(m_experimentalGroup);
    contentLayout->addWidget(m_killSwitchGroup);
    contentLayout->addStretch(1);

    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setOwnedWidget(std::move(content));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(scroll, 1);
    layout->addWidget(buttons);

    updateExperimentalVisibility();

    QScreen* dlgScreen = screen();
    if (!dlgScreen) {
        dlgScreen = QGuiApplication::primaryScreen();
    }
    const QRect available = dlgScreen ? dlgScreen->availableGeometry() : QRect(0, 0, 1280, 720);
    const int width = qBound(620, 480, available.width() - 48);
    const int height = qBound(720, 420, available.height() - 72);
    resize(width, height);
    setMaximumHeight(available.height() - 24);
}


bool SettingsDialog::confirmTunWarningIfNeeded()
{
    AppSettings& settings = AppSettings::instance();
    if (settings.experimentalTunWarningAccepted() || settings.tunWarningAccepted()) {
        return true;
    }

    const QString action = UiMessagePresenter::choose(
        this,
        tr("Experimental TUN mode"),
        tr("TUN mode is experimental and is not the recommended beta path.")
            + QStringLiteral("\n\n")
            + tr("Recommended for beta:\nXray system-proxy mode.\n\n"
                 "Continue with experimental TUN?"),
        UiMessageTone::Warning,
        {
            {QStringLiteral("continue"), tr("Continue"),
             UiMessageActionRole::Primary, true, false},
            {QStringLiteral("switch"), tr("Switch to Xray system proxy")},
            {QStringLiteral("cancel"), tr("Cancel"),
             UiMessageActionRole::Secondary, false, true},
        });

    if (action == QStringLiteral("switch")) {
        m_enableExperimentalTunCheck->setChecked(false);
        m_runtimeModeGroup->setValue(static_cast<int>(RuntimeMode::SystemProxyXray));
        return false;
    }
    if (action != QStringLiteral("continue")) {
        return false;
    }
    settings.setExperimentalTunWarningAccepted(true);
    settings.setTunWarningAccepted(true);
    return true;
}

bool SettingsDialog::confirmKillSwitchWarningIfNeeded()
{
    const QString action = UiMessagePresenter::choose(
        this,
        tr("Experimental kill switch"),
        tr("Kill switch is experimental and may block networking if it fails.")
            + QStringLiteral("\n\n")
            + tr("Make sure you know the recovery procedure before enabling it."),
        UiMessageTone::Warning,
        {
            {QStringLiteral("recovery"), tr("Show Recovery Instructions")},
            {QStringLiteral("enable"), tr("Enable"),
             UiMessageActionRole::Primary, true, false},
            {QStringLiteral("cancel"), tr("Cancel"),
             UiMessageActionRole::Secondary, false, true},
        });

    if (action == QStringLiteral("recovery")) {
        onShowKillSwitchRecovery();
        return false;
    }
    if (action != QStringLiteral("enable")) {
        return false;
    }
    return true;
}

void SettingsDialog::updateProxyEndpointLabel()
{
    m_proxyEndpointLabel->setText(
        QStringLiteral("127.0.0.1:%1").arg(m_mixedPortSpin->value()));
}

void SettingsDialog::refreshRoutingCombo()
{
    const QString activeId = m_routingManager.activeProfileId();
    const QVector<RoutingProfile> profiles = m_routingManager.profiles();
    QVector<ZaryaSelectorItem> items;
    items.reserve(profiles.size());
    for (const RoutingProfile& profile : profiles) {
        items.push_back({profile.id, profile.name, true});
    }
    m_routingProfileCombo->setItems(std::move(items), activeId);
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
    const QString activeId = m_dnsManager.activeProfileId();
    const QVector<DnsProfile> profiles = m_dnsManager.profiles();
    QVector<ZaryaSelectorItem> items;
    items.reserve(profiles.size());
    for (const DnsProfile& profile : profiles) {
        items.push_back({profile.id, profile.name, true});
    }
    m_dnsProfileCombo->setItems(std::move(items), activeId);
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
        UiMessagePresenter::warning(
            this, tr("Settings"), tr("Test URL must be a valid http or https URL."));
        return false;
    }

    bool languageChanged = false;
    const QString selectedLanguage = m_languageCombo->currentKey();
    if (selectedLanguage != LanguageManager::instance().currentLanguageCode()) {
        QString langError;
        if (!LanguageManager::instance().setLanguage(selectedLanguage, &langError)) {
            UiMessagePresenter::warning(this, tr("Settings"), langError);
            return false;
        }
        languageChanged = true;
    }

    const ThemeMode selectedTheme = themeModeFromString(m_themeCombo->currentKey());
    if (selectedTheme != ThemeManager::instance().mode()) {
        ThemeManager::instance().setMode(selectedTheme);
    }

    AppSettings& settings = AppSettings::instance();
    settings.setMixedPort(m_mixedPortSpin->value());
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
                UiMessagePresenter::warning(this, tr("Autostart"), autostartError);
                return false;
            }
        } else if (m_autostartManager->isEnabled()
                   && !m_autostartManager->disable(&autostartError)) {
            UiMessagePresenter::warning(this, tr("Autostart"), autostartError);
            return false;
        }
        settings.setStartAtLogin(wantAutostart);
    }

    const QString selectedRoutingId = m_routingProfileCombo->currentKey();
    if (!selectedRoutingId.isEmpty()) {
        m_routingManager.setActiveProfileId(selectedRoutingId);
        m_routingManager.save();
    }

    const QString selectedDnsId = m_dnsProfileCombo->currentKey();
    if (!selectedDnsId.isEmpty()) {
        m_dnsManager.setActiveProfileId(selectedDnsId);
        m_dnsManager.save();
    }

    const bool wantExperimentalTun = m_enableExperimentalTunCheck->isChecked();
    const RuntimeMode selectedMode =
        static_cast<RuntimeMode>(m_runtimeModeGroup->value());
    if (wantExperimentalTun || selectedMode == RuntimeMode::TunSingBoxExperimental) {
        if (!confirmTunWarningIfNeeded()) {
            return false;
        }
    }

    settings.setEnableExperimentalTun(wantExperimentalTun);
    settings.setRuntimeMode(selectedMode);
    settings.setTunUseActiveRoutingProfile(m_tunUseActiveRoutingCheck->isChecked());
    settings.setTunUseActiveDnsProfile(m_tunUseActiveDnsCheck->isChecked());
    settings.setTunEnableDnsHijack(m_tunEnableDnsHijackCheck->isChecked());
    settings.setTunDnsHijackMode(static_cast<TunDnsHijackMode>(
        m_tunDnsHijackModeCombo->currentKey().toInt()));
    settings.setTunPrivilegeMode(
        static_cast<TunPrivilegeMode>(m_tunPrivilegeModeGroup->value()));
    settings.setTunRequireLocalRuleSets(m_tunRequireLocalRuleSetsCheck->isChecked());

    const bool killSwitchEnabled = m_enableKillSwitchCheck->isChecked();
    if (killSwitchEnabled && !AppSettings::instance().enableExperimentalKillSwitch()) {
        if (!confirmKillSwitchWarningIfNeeded()) {
            return false;
        }
    }
    settings.setEnableExperimentalKillSwitch(killSwitchEnabled);
    settings.setKillSwitchMode(killSwitchEnabled ? KillSwitchMode::TunOnlyExperimental
                                                 : KillSwitchMode::Disabled);
    settings.setKillSwitchAllowLan(m_killSwitchAllowLanCheck->isChecked());
    settings.setKillSwitchAllowLoopback(m_killSwitchAllowLoopbackCheck->isChecked());
    settings.setKillSwitchBlockWhenTunStopped(true);
    settings.setKillSwitchAutoDisableOnCleanStop(
        m_killSwitchAutoDisableOnStopCheck->isChecked());

    const QString previousReleaseChannel = settings.releaseChannelKey();
    const bool previousShowExperimental = settings.showExperimentalFeatures();
    const RuntimeMode previousEffective = settings.effectiveRuntimeMode();

    settings.setReleaseChannelKey(m_releaseChannelCombo->currentKey());
    settings.setShowExperimentalFeatures(m_showExperimentalFeaturesCheck->isChecked());

    settings.setAppUpdateChannelKey(m_appUpdateChannelCombo->currentKey());
    settings.setCheckAppUpdatesOnStartup(m_checkAppUpdatesOnStartupCheck->isChecked());
    settings.setAppUpdateManifestUrl(m_appUpdateManifestUrlEdit->text());
    settings.setAllowUnsignedAppUpdates(m_allowUnsignedAppUpdatesCheck->isChecked());

    settings.setAllowCoreUpdateWithoutChecksum(m_allowCoreUpdateWithoutChecksumCheck->isChecked());
    settings.setAllowManageExternalCorePaths(m_allowManageExternalCorePathsCheck->isChecked());
    settings.setCoreBackupRetentionCount(m_coreBackupRetentionSpin->value());
    settings.setGithubApiTimeoutSeconds(m_githubApiTimeoutSpin->value());
    settings.setCheckCoreUpdatesOnStartup(m_checkCoreUpdatesOnStartupCheck->isChecked());

    if (languageChanged) {
        UiMessagePresenter::information(
            this, tr("Settings"),
            tr("Language will be fully applied after restart."));
    }

    const RuntimeMode newEffective = settings.effectiveRuntimeMode();
    if ((previousShowExperimental && !settings.showExperimentalFeatures())
        || (previousEffective == RuntimeMode::TunSingBoxExperimental
            && newEffective == RuntimeMode::SystemProxyXray
            && settings.configuredRuntimeMode() == RuntimeMode::TunSingBoxExperimental)) {
        UiMessagePresenter::warning(
            this, tr("Experimental features disabled"),
            tr("Experimental features are disabled. Runtime will use Xray system-proxy mode."));
    } else if (previousReleaseChannel != settings.releaseChannelKey()
               && FeaturePolicy::isStableLikeChannel(
                   FeaturePolicy::releaseChannelFromString(settings.releaseChannelKey()))
               && !settings.showExperimentalFeatures()) {
        UiMessagePresenter::information(
            this, tr("Stable scope"),
            tr("Experimental TUN, helper, and kill switch controls are hidden. Recovery actions "
               "remain available when needed."));
    }

    return true;
}

void SettingsDialog::updateKillSwitchControls()
{
    const bool enabled = m_enableKillSwitchCheck->isChecked();
    const bool helperMode = m_tunPrivilegeModeGroup->value()
        == static_cast<int>(TunPrivilegeMode::HelperExperimental);
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
        UiMessagePresenter::warning(this, tr("Kill switch"), error);
        return;
    }
    QJsonObject payload;
    if (!m_helperManager->killSwitchCheckSupport(&payload, &error)) {
        UiMessagePresenter::warning(this, tr("Kill switch"), error);
        return;
    }
    UiMessagePresenter::information(
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
    UiMessagePresenter::warning(
        this, tr("Kill switch"),
        tr("The Windows WFP kill switch is experimental.\n\n"
           "It will install temporary Zarya-owned WFP filters to block outbound connections "
           "except loopback and the selected proxy server.\n\n"
           "zarya-helper must be running as Administrator.\n\n"
           "Kill switch is enabled automatically when you start TUN mode with kill switch "
           "enabled."));
#else
    UiMessagePresenter::information(
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
        UiMessagePresenter::warning(this, tr("Kill switch"), error);
        return;
    }
    if (!m_helperManager->killSwitchDisable(&error)) {
        UiMessagePresenter::warning(this, tr("Kill switch"), error);
        return;
    }
    UiMessagePresenter::information(
        this, tr("Kill switch"), tr("Kill switch disabled."));
}

void SettingsDialog::onShowKillSwitchRecovery()
{
    UiMessagePresenter::information(
        this, tr("Kill switch recovery"),
        HelperProcessManager::recoveryInstructionsText());
}

void SettingsDialog::onStartHelper()
{
    if (!m_helperManager) {
        return;
    }
    QString error;
    if (!m_helperManager->startHelperDevMode(&error)) {
        UiMessagePresenter::warning(this, tr("Helper"), error);
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
        UiMessagePresenter::warning(this, tr("Helper"), error);
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
        UiMessagePresenter::warning(this, tr("Helper"), error);
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
#if defined(Q_OS_WIN)
        if (InstallationInfo::detect() == InstallationMode::Installed
            && WindowsInstallInfo::isAvailable()) {
            if (WindowsInstallInfo::helperServiceInstalled()) {
                m_helperServiceStatusLabel->setText(
                    tr("Helper service: %1").arg(WindowsInstallInfo::helperServiceState()));
            } else {
                m_helperServiceStatusLabel->setText(
                    tr("Helper service is not installed. Optional — only needed for experimental "
                       "TUN/kill switch."));
            }
            return;
        }
#endif
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
        UiMessagePresenter::warning(
            this,
            tr("Install helper service"),
            tr("Installing Zarya Helper service requires Administrator privileges.")
                + QStringLiteral("\n\n") + error);
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
        UiMessagePresenter::warning(this, tr("Uninstall helper service"), error);
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
        UiMessagePresenter::warning(this, tr("Start helper service"), error);
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
        UiMessagePresenter::warning(this, tr("Stop helper service"), error);
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
        UiMessagePresenter::warning(this, tr("Restart helper service"), error);
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
    QString message = QString::fromUtf8(process.readAllStandardOutput());
    if (process.exitCode() != 0) {
        message += QStringLiteral("\n\n")
            + QString::fromUtf8(process.readAllStandardError());
        UiMessagePresenter::warning(this, tr("Helper self-test"), message);
    } else {
        UiMessagePresenter::information(this, tr("Helper self-test"), message);
    }
}

void SettingsDialog::onShowServiceRecovery()
{
    const QString text = m_serviceManager ? m_serviceManager->recoveryInstructions()
                                          : HelperProcessManager::recoveryInstructionsText();
    UiMessagePresenter::information(this, tr("Helper service recovery"), text);
}

void SettingsDialog::updateExperimentalVisibility()
{
    const ReleaseChannel channel = FeaturePolicy::releaseChannelFromString(
        m_releaseChannelCombo->currentKey());
    const bool visible = m_showExperimentalFeaturesCheck->isChecked();
    m_experimentalGroup->setVisible(visible);
    m_killSwitchGroup->setVisible(visible);
    m_experimentalGatePanel->setVisible(!visible);
    m_showExperimentalFeaturesCheck->setVisible(!FeaturePolicy::isStableLikeChannel(channel));
}

void SettingsDialog::onShowExperimentalFeatures()
{
    if (!UiMessagePresenter::confirm(
            this,
            tr("Experimental features"),
            tr("Experimental features may break networking and are not part of stable support.")
                + QStringLiteral("\n\n")
                + tr("TUN, zarya-helper, and kill switch are experimental. Use Xray "
                     "system-proxy mode for the recommended stable path."),
            tr("Continue"))) {
        return;
    }
    m_showExperimentalFeaturesCheck->setChecked(true);
    updateExperimentalVisibility();
}

} // namespace zarya
