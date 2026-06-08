#pragma once

#include "domain/Profile.h"
#include "domain/Subscription.h"
#include "runtime/RuntimeBackendType.h"

#include <QString>
#include <QVector>

namespace zarya {

class CoreBinaryManager;

struct FirstRunState {
    QString routingProfileId;
    QString dnsProfileId;
    RuntimeMode runtimeMode = RuntimeMode::SystemProxyXray;
    bool tunWarningAccepted = false;
    bool startProfileOnFinish = false;
    QVector<Profile> importedProfiles;
    QVector<Subscription> addedSubscriptions;

    static bool shouldShowWizard(const CoreBinaryManager& coreManager, int profileCount,
                                 int subscriptionCount);
    static void applyDefaults(FirstRunState* state);
};

} // namespace zarya
