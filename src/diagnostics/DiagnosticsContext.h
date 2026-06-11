#pragma once

#include "domain/Profile.h"

#include <QDateTime>
#include <QVector>

namespace zarya {

class AppController;
class CoreBinaryManager;
class CoreManager;
class DnsManager;
class GeoDataManager;
class HelperProcessManager;
class IHelperServiceManager;
class RoutingManager;
class RuleSetManager;
class SystemProxyController;
class XrayAdapter;

struct DiagnosticsContext {
    QVector<Profile> profiles;
    Profile selectedProfile;
    bool hasSelectedProfile = false;

    CoreManager* coreManager = nullptr;
    CoreBinaryManager* coreBinaryManager = nullptr;
    AppController* appController = nullptr;
    SystemProxyController* systemProxy = nullptr;
    RoutingManager* routingManager = nullptr;
    DnsManager* dnsManager = nullptr;
    GeoDataManager* geoDataManager = nullptr;
    RuleSetManager* ruleSetManager = nullptr;
    HelperProcessManager* helper = nullptr;
    IHelperServiceManager* helperService = nullptr;
    XrayAdapter* xrayAdapter = nullptr;

    QDateTime appStartedAt;
    bool systemTrayAvailable = false;
};

} // namespace zarya
