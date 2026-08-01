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
class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaNumberField;
class ZaryaSelector;
class ZaryaTextField;

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
    void updateProxyEndpointLabel();
    void refreshRoutingCombo();
    void refreshDnsCombo();
    void refreshHelperServiceUi();
    void updateExperimentalVisibility();
    void onShowExperimentalFeatures();

private:
    bool validateAndSave();

    ZaryaTextField* m_xrayPathEdit = nullptr;
    ZaryaNumberField* m_mixedPortSpin = nullptr;
    ZaryaCheckBox* m_autoEnableSystemProxyCheck = nullptr;
    ZaryaCheckBox* m_restoreProxyOnExitCheck = nullptr;
    ZaryaBodyText* m_proxyEndpointLabel = nullptr;
    ZaryaBodyText* m_proxyBackendLabel = nullptr;
    ZaryaBodyText* m_proxySupportLabel = nullptr;
    ZaryaBodyText* m_proxyLimitationsLabel = nullptr;
    ZaryaBodyText* m_linuxDesktopLabel = nullptr;
    ZaryaCheckBox* m_macApplyAllServicesCheck = nullptr;
    ZaryaTextField* m_macPreferredServiceEdit = nullptr;

    ZaryaTextField* m_testUrlEdit = nullptr;
    ZaryaNumberField* m_tcpTimeoutSpin = nullptr;
    ZaryaNumberField* m_realDelayTimeoutSpin = nullptr;
    ZaryaNumberField* m_maxConcurrentTestsSpin = nullptr;
    ZaryaCheckBox* m_skipTcpBeforeRealDelayCheck = nullptr;

    ZaryaCheckBox* m_minimizeToTrayOnCloseCheck = nullptr;
    ZaryaCheckBox* m_minimizeToTrayOnMinimizeCheck = nullptr;
    ZaryaCheckBox* m_showTrayNotificationsCheck = nullptr;
    ZaryaCheckBox* m_confirmExitWhileRunningCheck = nullptr;

    ZaryaSelector* m_languageCombo = nullptr;
    ZaryaSelector* m_themeCombo = nullptr;
    ZaryaSelector* m_routingProfileCombo = nullptr;
    ZaryaSelector* m_dnsProfileCombo = nullptr;

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

    ZaryaSelector* m_appUpdateChannelCombo = nullptr;
    ZaryaCheckBox* m_checkAppUpdatesOnStartupCheck = nullptr;
    ZaryaTextField* m_appUpdateManifestUrlEdit = nullptr;
    ZaryaCheckBox* m_allowUnsignedAppUpdatesCheck = nullptr;

    ZaryaSelector* m_releaseChannelCombo = nullptr;
    ZaryaCheckBox* m_showExperimentalFeaturesCheck = nullptr;
    QWidget* m_experimentalGatePanel = nullptr;
    ZaryaActionButton* m_showExperimentalFeaturesButton = nullptr;
    QGroupBox* m_experimentalGroup = nullptr;
    QGroupBox* m_killSwitchGroup = nullptr;

    ZaryaCheckBox* m_allowCoreUpdateWithoutChecksumCheck = nullptr;
    ZaryaCheckBox* m_allowManageExternalCorePathsCheck = nullptr;
    ZaryaNumberField* m_coreBackupRetentionSpin = nullptr;
    ZaryaNumberField* m_githubApiTimeoutSpin = nullptr;
    ZaryaCheckBox* m_checkCoreUpdatesOnStartupCheck = nullptr;

    ZaryaCheckBox* m_startAtLoginCheck = nullptr;
    ZaryaCheckBox* m_startMinimizedToTrayCheck = nullptr;
    ZaryaCheckBox* m_autoStartLastProfileCheck = nullptr;
    ZaryaCheckBox* m_autoEnableProxyAfterAutoStartCheck = nullptr;
    ZaryaNumberField* m_autoStartDelaySpin = nullptr;
    ZaryaBodyText* m_autostartBackendLabel = nullptr;

    RoutingManager& m_routingManager;
    DnsManager& m_dnsManager;
    HelperProcessManager* m_helperManager = nullptr;
    IHelperServiceManager* m_serviceManager = nullptr;
    std::unique_ptr<IAutostartManager> m_autostartManager;
};

} // namespace zarya
