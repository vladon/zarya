#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace zarya {

class RoutingManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(RoutingManager& routingManager, QWidget* parent = nullptr);

private slots:
    void onBrowseXray();
    void onManageRoutingProfiles();
    void updateHttpEndpointLabel();
    void refreshRoutingCombo();

private:
    bool validateAndSave();

    QLineEdit* m_xrayPathEdit = nullptr;
    QSpinBox* m_socksPortSpin = nullptr;
    QSpinBox* m_httpPortSpin = nullptr;
    QCheckBox* m_autoEnableSystemProxyCheck = nullptr;
    QCheckBox* m_restoreProxyOnExitCheck = nullptr;
    QLabel* m_httpEndpointLabel = nullptr;

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
    RoutingManager& m_routingManager;
};

} // namespace zarya
