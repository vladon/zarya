#pragma once

#include "domain/Profile.h"
#include "killswitch/KillSwitchRuleSet.h"

#include <QJsonObject>
#include <QString>

namespace zarya {

struct KillSwitchPayloadResult {
    KillSwitchRuleSet rules;
    QString resolveWarning;
    bool resolutionFailed = false;
};

class KillSwitchPayloadBuilder {
public:
    static KillSwitchPayloadResult build(const Profile& profile, bool allowLan,
                                         bool allowLoopback, bool blockDirectDns);
    static QJsonObject toJson(const KillSwitchRuleSet& rules);
};

} // namespace zarya
