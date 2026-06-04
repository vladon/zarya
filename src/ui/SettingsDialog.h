#pragma once

#include "platform/IAutostartManager.h"

#include <QDialog>

#include <memory>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

namespace zarya {

class DnsManager;
class HelperProcessManager;
class ISystemProxyManager;
class RoutingManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(RoutingManager& routingManager, DnsManager& dnsManager,
                            HelperProcessManager* helperManager = nullptr,
                            QWidget* parent = nullptr);

private slots:
    void onBrowseXray();
    void onBrowseSingBox();
    void onStartHelper();
    void onConnectHelper();
    void onCheckHelperStatus();
    bool confirmTunWarningIfNeeded();
    void onManageRoutingProfiles();
    void onManageDnsProfiles();
    void updateHttpEndpointLabel();
    void refreshRoutingCombo();
    void refreshDnsCombo();

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
    QLabel* m_helperStatusLabel = nullptr;
    QPushButton* m_startHelperButton = nullptr;
    QPushButton* m_connectHelperButton = nullptr;
    QPushButton* m_checkHelperStatusButton = nullptr;

    QCheckBox* m_startAtLoginCheck = nullptr;
    QCheckBox* m_startMinimizedToTrayCheck = nullptr;
    QCheckBox* m_autoStartLastProfileCheck = nullptr;
    QCheckBox* m_autoEnableProxyAfterAutoStartCheck = nullptr;
    QSpinBox* m_autoStartDelaySpin = nullptr;
    QLabel* m_autostartBackendLabel = nullptr;

    RoutingManager& m_routingManager;
    DnsManager& m_dnsManager;
    HelperProcessManager* m_helperManager = nullptr;
    std::unique_ptr<IAutostartManager> m_autostartManager;
};

} // namespace zarya
