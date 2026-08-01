#pragma once

#include "platform/IAutostartManager.h"

#include <QDialog>

#include <memory>

namespace zarya {

class DnsManager;
class HelperProcessManager;
class IHelperServiceManager;
class ISystemProxyManager;
class RoutingManager;
class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaFormSection;
class ZaryaNumberField;
class ZaryaRadioGroup;
class ZaryaSelector;
class ZaryaTextField;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(RoutingManager& routingManager, DnsManager& dnsManager,
                            HelperProcessManager* helperManager = nullptr,
                            IHelperServiceManager* serviceManager = nullptr,
                            QWidget* parent = nullptr);

private Q_SLOTS:
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

    ZaryaCheckBox* m_enableExperimentalTunCheck = nullptr;
    ZaryaRadioGroup* m_runtimeModeGroup = nullptr;
    ZaryaTextField* m_singBoxPathEdit = nullptr;
    ZaryaCheckBox* m_tunUseActiveRoutingCheck = nullptr;
    ZaryaCheckBox* m_tunUseActiveDnsCheck = nullptr;
    ZaryaCheckBox* m_tunEnableDnsHijackCheck = nullptr;
    ZaryaSelector* m_tunDnsHijackModeCombo = nullptr;
    ZaryaRadioGroup* m_tunPrivilegeModeGroup = nullptr;
    ZaryaBodyText* m_helperBackendLabel = nullptr;
    ZaryaBodyText* m_helperServiceStatusLabel = nullptr;
    ZaryaBodyText* m_helperStatusLabel = nullptr;
    ZaryaBodyText* m_helperServiceWarningLabel = nullptr;
    ZaryaActionButton* m_installServiceButton = nullptr;
    ZaryaActionButton* m_uninstallServiceButton = nullptr;
    ZaryaActionButton* m_startServiceButton = nullptr;
    ZaryaActionButton* m_stopServiceButton = nullptr;
    ZaryaActionButton* m_restartServiceButton = nullptr;
    ZaryaActionButton* m_startHelperButton = nullptr;
    ZaryaActionButton* m_connectHelperButton = nullptr;
    ZaryaActionButton* m_checkHelperStatusButton = nullptr;
    ZaryaActionButton* m_serviceSelfTestButton = nullptr;
    ZaryaActionButton* m_serviceRecoveryButton = nullptr;
    ZaryaCheckBox* m_recoverKillSwitchOnUninstallCheck = nullptr;

    ZaryaCheckBox* m_tunRequireLocalRuleSetsCheck = nullptr;
    ZaryaBodyText* m_ruleSetDirLabel = nullptr;

    ZaryaCheckBox* m_enableKillSwitchCheck = nullptr;
    ZaryaBodyText* m_killSwitchModeLabel = nullptr;
    ZaryaCheckBox* m_killSwitchAllowLanCheck = nullptr;
    ZaryaCheckBox* m_killSwitchAllowLoopbackCheck = nullptr;
    ZaryaCheckBox* m_killSwitchAllowProxyCheck = nullptr;
    ZaryaCheckBox* m_killSwitchAutoDisableOnStopCheck = nullptr;
    ZaryaCheckBox* m_killSwitchKeepActiveAfterStopCheck = nullptr;
    ZaryaBodyText* m_killSwitchBackendLabel = nullptr;
    ZaryaBodyText* m_killSwitchWarningLabel = nullptr;
    ZaryaActionButton* m_testKillSwitchButton = nullptr;
    ZaryaActionButton* m_enableKillSwitchButton = nullptr;
    ZaryaActionButton* m_disableKillSwitchButton = nullptr;
    ZaryaActionButton* m_killSwitchRecoveryButton = nullptr;

    ZaryaSelector* m_appUpdateChannelCombo = nullptr;
    ZaryaCheckBox* m_checkAppUpdatesOnStartupCheck = nullptr;
    ZaryaTextField* m_appUpdateManifestUrlEdit = nullptr;
    ZaryaCheckBox* m_allowUnsignedAppUpdatesCheck = nullptr;

    ZaryaSelector* m_releaseChannelCombo = nullptr;
    ZaryaCheckBox* m_showExperimentalFeaturesCheck = nullptr;
    QWidget* m_experimentalGatePanel = nullptr;
    ZaryaActionButton* m_showExperimentalFeaturesButton = nullptr;
    ZaryaFormSection* m_experimentalGroup = nullptr;
    ZaryaFormSection* m_killSwitchGroup = nullptr;

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
