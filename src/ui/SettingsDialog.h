#pragma once

#include "platform/IAutostartManager.h"

#include <QDialog>

#include <memory>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

namespace zarya {

class DnsManager;
class HelperProcessManager;
class IHelperServiceManager;
class ISystemProxyManager;
class RoutingManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(RoutingManager& routingManager, DnsManager& dnsManager,
                            HelperProcessManager* helperManager = nullptr,
                            IHelperServiceManager* serviceManager = nullptr,
                            QWidget* parent = nullptr);

private slots:
    void onBrowseXray();
    void onBrowseSingBox();
    void onStartHelper();
    void onConnectHelper();
    void onCheckHelperStatus();
    void onInstallService();
    void onUninstallService();
    void onStartService();
    void onStopService();
    void onRestartService();
    void onServiceSelfTest();
    void onShowServiceRecovery();
    void onTestKillSwitchSupport();
    void onEnableKillSwitchNow();
    void onDisableKillSwitchNow();
    void onShowKillSwitchRecovery();
    bool confirmTunWarningIfNeeded();
    bool confirmKillSwitchWarningIfNeeded();
    void updateKillSwitchControls();
    void onManageRoutingProfiles();
    void onManageDnsProfiles();
    void updateHttpEndpointLabel();
    void refreshRoutingCombo();
    void refreshDnsCombo();
    void refreshHelperServiceUi();
    void updateExperimentalVisibility();
    void onShowExperimentalFeatures();

private:
    bool validateAndSave();

    QLineEdit* m_xrayPathEdit = nullptr;
    QSpinBox* m_socksPortSpin = nullptr;
    QSpinBox* m_httpPortSpin = nullptr;
    QCheckBox* m_autoEnableSystemProxyCheck = nullptr;
    QCheckBox* m_restoreProxyOnExitCheck = nullptr;
    QLabel* m_httpEndpointLabel = nullptr;
    QLabel* m_proxyBackendLabel = nullptr;
    QLabel* m_proxySupportLabel = nullptr;
    QLabel* m_proxyLimitationsLabel = nullptr;
    QLabel* m_linuxDesktopLabel = nullptr;
    QCheckBox* m_macApplyAllServicesCheck = nullptr;
    QLineEdit* m_macPreferredServiceEdit = nullptr;

    QLineEdit* m_testUrlEdit = nullptr;
    QSpinBox* m_tcpTimeoutSpin = nullptr;
    QSpinBox* m_realDelayTimeoutSpin = nullptr;
    QSpinBox* m_maxConcurrentTestsSpin = nullptr;
    QCheckBox* m_skipTcpBeforeRealDelayCheck = nullptr;

    QCheckBox* m_minimizeToTrayOnCloseCheck = nullptr;
    QCheckBox* m_minimizeToTrayOnMinimizeCheck = nullptr;
    QCheckBox* m_showTrayNotificationsCheck = nullptr;
    QCheckBox* m_confirmExitWhileRunningCheck = nullptr;

    QComboBox* m_languageCombo = nullptr;
    QComboBox* m_routingProfileCombo = nullptr;
    QComboBox* m_dnsProfileCombo = nullptr;

    QCheckBox* m_enableExperimentalTunCheck = nullptr;
    QRadioButton* m_systemProxyRuntimeRadio = nullptr;
    QRadioButton* m_tunRuntimeRadio = nullptr;
    QLineEdit* m_singBoxPathEdit = nullptr;
    QCheckBox* m_tunUseActiveRoutingCheck = nullptr;
    QCheckBox* m_tunUseActiveDnsCheck = nullptr;
    QCheckBox* m_tunEnableDnsHijackCheck = nullptr;
    QComboBox* m_tunDnsHijackModeCombo = nullptr;
    QRadioButton* m_tunDirectGuiRadio = nullptr;
    QRadioButton* m_tunHelperRadio = nullptr;
    QLabel* m_helperBackendLabel = nullptr;
    QLabel* m_helperServiceStatusLabel = nullptr;
    QLabel* m_helperStatusLabel = nullptr;
    QLabel* m_helperServiceWarningLabel = nullptr;
    QPushButton* m_installServiceButton = nullptr;
    QPushButton* m_uninstallServiceButton = nullptr;
    QPushButton* m_startServiceButton = nullptr;
    QPushButton* m_stopServiceButton = nullptr;
    QPushButton* m_restartServiceButton = nullptr;
    QPushButton* m_startHelperButton = nullptr;
    QPushButton* m_connectHelperButton = nullptr;
    QPushButton* m_checkHelperStatusButton = nullptr;
    QPushButton* m_serviceSelfTestButton = nullptr;
    QPushButton* m_serviceRecoveryButton = nullptr;
    QCheckBox* m_recoverKillSwitchOnUninstallCheck = nullptr;

    QCheckBox* m_tunRequireLocalRuleSetsCheck = nullptr;
    QLabel* m_ruleSetDirLabel = nullptr;

    QCheckBox* m_enableKillSwitchCheck = nullptr;
    QLabel* m_killSwitchModeLabel = nullptr;
    QCheckBox* m_killSwitchAllowLanCheck = nullptr;
    QCheckBox* m_killSwitchAllowLoopbackCheck = nullptr;
    QCheckBox* m_killSwitchAllowProxyCheck = nullptr;
    QCheckBox* m_killSwitchAutoDisableOnStopCheck = nullptr;
    QCheckBox* m_killSwitchKeepActiveAfterStopCheck = nullptr;
    QLabel* m_killSwitchBackendLabel = nullptr;
    QLabel* m_killSwitchWarningLabel = nullptr;
    QPushButton* m_testKillSwitchButton = nullptr;
    QPushButton* m_enableKillSwitchButton = nullptr;
    QPushButton* m_disableKillSwitchButton = nullptr;
    QPushButton* m_killSwitchRecoveryButton = nullptr;

    QComboBox* m_appUpdateChannelCombo = nullptr;
    QCheckBox* m_checkAppUpdatesOnStartupCheck = nullptr;
    QLineEdit* m_appUpdateManifestUrlEdit = nullptr;
    QCheckBox* m_allowUnsignedAppUpdatesCheck = nullptr;

    QComboBox* m_releaseChannelCombo = nullptr;
    QCheckBox* m_showExperimentalFeaturesCheck = nullptr;
    QWidget* m_experimentalGatePanel = nullptr;
    QPushButton* m_showExperimentalFeaturesButton = nullptr;
    QGroupBox* m_experimentalGroup = nullptr;
    QGroupBox* m_killSwitchGroup = nullptr;

    QCheckBox* m_allowCoreUpdateWithoutChecksumCheck = nullptr;
    QCheckBox* m_allowManageExternalCorePathsCheck = nullptr;
    QSpinBox* m_coreBackupRetentionSpin = nullptr;
    QSpinBox* m_githubApiTimeoutSpin = nullptr;
    QCheckBox* m_checkCoreUpdatesOnStartupCheck = nullptr;

    QCheckBox* m_startAtLoginCheck = nullptr;
    QCheckBox* m_startMinimizedToTrayCheck = nullptr;
    QCheckBox* m_autoStartLastProfileCheck = nullptr;
    QCheckBox* m_autoEnableProxyAfterAutoStartCheck = nullptr;
    QSpinBox* m_autoStartDelaySpin = nullptr;
    QLabel* m_autostartBackendLabel = nullptr;

    RoutingManager& m_routingManager;
    DnsManager& m_dnsManager;
    HelperProcessManager* m_helperManager = nullptr;
    IHelperServiceManager* m_serviceManager = nullptr;
    std::unique_ptr<IAutostartManager> m_autostartManager;
};

} // namespace zarya
