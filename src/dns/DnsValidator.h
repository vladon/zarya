#pragma once

#include "domain/DnsProfile.h"

#include <QStringList>

namespace zarya {

class DnsValidator {
public:
    static QStringList warnings(const DnsProfile& profile);
    static QStringList interactionWarnings(const DnsProfile& dnsProfile, bool routingUsesGeoData);
};

} // namespace zarya
