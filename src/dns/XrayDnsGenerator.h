#pragma once

#include "domain/DnsProfile.h"

#include <QJsonObject>

namespace zarya {

class XrayDnsGenerator {
public:
    QJsonObject generate(const DnsProfile& profile) const;
    bool shouldGenerateDnsObject(const DnsProfile& profile) const;
    int enabledServerCount(const DnsProfile& profile) const;
};

} // namespace zarya
