#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace zarya {

class StatusBadge;

#if defined(ZARYA_DESKTOP_APP_UI)
class StatusConfiguredStrip;
#endif

struct StatusDashboardModel {
    bool configured = false;
    bool xrayInstalled = false;
    bool hasProfiles = false;
    bool running = false;
    QString runtimeText;
    QString recommendedRuntimeText;
    bool experimentalRuntimeActive = false;
    QString profileName;
    QString routingText;
    QString dnsText;
    QString systemProxyText;
    QString coreText;
    QString localEndpoint;
};

class StatusDashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusDashboardWidget(QWidget* parent = nullptr);

    void updateModel(const StatusDashboardModel& model);

Q_SIGNALS:
    void openCoreManagerRequested();
    void addProfileRequested();
    void addSubscriptionRequested();
    void runSetupRequested();
    void pasteLinkRequested();
    void importBackupRequested();
    void startRequested();
    void stopRequested();
    void testRequested();
    void updateSubscriptionsRequested();
    void openLogsRequested();
    void createDiagnosticsRequested();
    void updateAllSubscriptionsRequested();

private:
    void showConfigured(const StatusDashboardModel& model);
    void showUnconfigured(const StatusDashboardModel& model);

    QWidget* m_unconfiguredPanel = nullptr;
    QLabel* m_unconfiguredStepsLabel = nullptr;
#if defined(ZARYA_DESKTOP_APP_UI)
    StatusConfiguredStrip* m_configuredStrip = nullptr;
#else
    QWidget* m_configuredPanel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_detailLabel = nullptr;
    StatusBadge* m_runtimeBadge = nullptr;
    QPushButton* m_primaryButton = nullptr;
    QPushButton* m_secondaryButton = nullptr;
#endif
};

} // namespace zarya
