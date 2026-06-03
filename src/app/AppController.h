#pragma once

#include "domain/DnsProfile.h"
#include "domain/Profile.h"
#include "domain/RoutingProfile.h"

#include <QObject>
#include <functional>

class QWidget;

namespace zarya {

class CoreManager;
class DnsManager;
class GeoDataManager;
class RoutingManager;
class SystemProxyController;
class TestManager;
class XrayAdapter;

class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(CoreManager* coreManager, SystemProxyController* systemProxy,
                           XrayAdapter* xrayAdapter, TestManager* testManager,
                           RoutingManager* routingManager, GeoDataManager* geoDataManager,
                           DnsManager* dnsManager, QObject* parent = nullptr);

    void setDialogParent(QWidget* parent);
    void setAfterCoreStartedCallback(std::function<void()> callback);
    void setSaveApplicationStateCallback(std::function<bool(QString*)> callback);
    void setOpenGeoDataManagerCallback(std::function<void()> callback);
    void setOpenDnsProfilesCallback(std::function<void()> callback);

    bool startProfile(const Profile& profile, bool fromAutostart = false);
    bool lastStartWasAutostart() const;
    bool stopCurrentProfile();
    bool isCoreRunning() const;

    bool enableSystemProxyManual();
    bool restoreSystemProxyManual();
    bool restoreSystemProxyAutomatic();

    void requestQuit();
    bool safeShutdown(bool proxyExitAnyway = false);

signals:
    void logLine(const QString& line);
    void coreStateChanged(bool running);
    void proxyStateChanged();
    void showMainWindowRequested();
    void hideMainWindowRequested();
    void quitApproved();
    void quitBlocked(const QString& reason);

private:
    bool confirmSystemProxyChangeIfNeeded() const;
    bool confirmGeoDataIfNeeded(const RoutingProfile& routingProfile);
    bool confirmDnsGeoDataIfNeeded(const DnsProfile& dnsProfile);
    bool confirmDnsWarningsIfNeeded(const DnsProfile& dnsProfile,
                                    const RoutingProfile& routingProfile);
    void logGeoDataContext();
    bool writeConfigFile(const QString& path, const QJsonObject& config, QString* error) const;
    QString configPathFor(CoreType type) const;
    bool attemptProxyRestoreOnShutdown(QString* error);

    CoreManager* m_coreManager = nullptr;
    SystemProxyController* m_systemProxy = nullptr;
    XrayAdapter* m_xrayAdapter = nullptr;
    TestManager* m_testManager = nullptr;
    RoutingManager* m_routingManager = nullptr;
    GeoDataManager* m_geoDataManager = nullptr;
    DnsManager* m_dnsManager = nullptr;
    QWidget* m_dialogParent = nullptr;
    std::function<void()> m_afterCoreStarted;
    std::function<bool(QString*)> m_saveApplicationState;
    std::function<void()> m_openGeoDataManager;
    std::function<void()> m_openDnsProfiles;
    bool m_lastStartWasAutostart = false;
};

} // namespace zarya
