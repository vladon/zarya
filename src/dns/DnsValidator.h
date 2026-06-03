#pragma once

#include "domain/DnsProfile.h"

#include <QStringList>

namespace zarya {

class DnsValidator {
public:
    static QStringList warnings(const DnsProfile& profile);
    static QStringList interactionWarnings(const DnsProfile& dnsProfile,
                                           const QString& routingDomainStrategy,
                                           bool routingUsesGeoData);
};

} // namespace zarya
