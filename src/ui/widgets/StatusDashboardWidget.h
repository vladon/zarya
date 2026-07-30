#pragma once

#include <QString>
#include <QWidget>

namespace zarya {

class StatusConfiguredStrip;
class StatusUnconfiguredPanel;

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

    StatusUnconfiguredPanel* m_unconfiguredPanel = nullptr;
    StatusConfiguredStrip* m_configuredStrip = nullptr;
};

} // namespace zarya
