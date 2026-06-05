#pragma once

#include "domain/DnsProfile.h"
#include "domain/RoutingProfile.h"
#include "rulesets/RuleSetStatus.h"

#include <QString>
#include <QVector>

namespace zarya {

struct RequiredRuleSet {
    QString normalizedId;
    QString tag;
    QString originalValue;
    QString sourceArea;
    bool available = false;
    QString localPath;
    QString warning;
    RuleSetStatus catalogStatus = RuleSetStatus::Unknown;
};

class RequiredRuleSetDetector {
public:
    static QVector<RequiredRuleSet> detect(const RoutingProfile& routingProfile,
                                           const DnsProfile& dnsProfile);
};

} // namespace zarya
