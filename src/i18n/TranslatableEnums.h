#pragma once

#include "domain/CoreType.h"
#include "domain/DnsProfileMode.h"
#include "domain/DnsQueryStrategy.h"
#include "domain/ProtocolType.h"
#include "domain/RoutingMode.h"
#include "domain/SubscriptionStatus.h"
#include "errors/AppError.h"
#include "geodata/GeoDataFileStatus.h"
#include "killswitch/KillSwitchState.h"
#include "rulesets/RuleSetStatus.h"
#include "runtime/RuntimeBackendType.h"
#include "testing/TestStatus.h"

#include <QString>

namespace zarya {

class TranslatableEnums {
public:
    static QString trProtocolType(ProtocolType type);
    static QString trCoreType(CoreType type);
    static QString trRuntimeMode(RuntimeMode mode);
    static QString trRuntimeState(RuntimeState state);
    static QString trSubscriptionStatus(SubscriptionStatus status);
    static QString trTestStatus(TestStatus status);
    static QString trRoutingMode(RoutingMode mode);
    static QString trRoutingAction(RoutingAction action);
    static QString trRoutingRuleType(RoutingRuleType type);
    static QString trDnsProfileMode(DnsProfileMode mode);
    static QString trDnsQueryStrategy(DnsQueryStrategy strategy);
    static QString trGeoDataStatus(GeoDataStatus status);
    static QString trRuleSetStatus(RuleSetStatus status);
    static QString trKillSwitchStatus(KillSwitchStatus status);
    static QString trErrorSeverity(ErrorSeverity severity);
};

} // namespace zarya
