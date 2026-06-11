#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace zarya {

class StatusBadge;

struct StatusDashboardModel {
    bool configured = false;
    bool running = false;
    QString runtimeText;
    QString recommendedRuntimeText;
    bool experimentalRuntimeActive = false;
    QString profileName;
    QString routingText;
    QString dnsText;
    QString systemProxyText;
    QString coreText;
    QString httpEndpoint;
    QString socksEndpoint;
};

class StatusDashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusDashboardWidget(QWidget* parent = nullptr);

    void updateModel(const StatusDashboardModel& model);

signals:
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
    void showUnconfigured();

    QWidget* m_unconfiguredPanel = nullptr;
    QWidget* m_configuredPanel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_detailLabel = nullptr;
    StatusBadge* m_runtimeBadge = nullptr;
    QPushButton* m_primaryButton = nullptr;
    QPushButton* m_secondaryButton = nullptr;
};

} // namespace zarya
