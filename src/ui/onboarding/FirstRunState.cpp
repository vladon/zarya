#include "ui/onboarding/FirstRunState.h"

#include "cores/CoreBinaryManager.h"
#include "domain/DnsProfileMode.h"
#include "domain/RoutingMode.h"
#include "storage/AppSettings.h"

#include <QFile>

namespace zarya {

bool FirstRunState::shouldShowWizard(const CoreBinaryManager& coreManager, int profileCount,
                                     int subscriptionCount)
{
    if (!AppSettings::instance().firstRunCompleted()) {
        return true;
    }
    const CoreInfo xray = coreManager.infoFor(CoreType::Xray);
    const bool hasXray = xray.exists;
    return !hasXray && profileCount == 0 && subscriptionCount == 0;
}

void FirstRunState::applyDefaults(FirstRunState* state)
{
    if (!state) {
        return;
    }
    state->routingProfileId = RoutingBuiltinIds::bypassLan();
    state->dnsProfileId = DnsBuiltinIds::systemDns();
    state->runtimeMode = RuntimeMode::SystemProxyXray;
}

} // namespace zarya
