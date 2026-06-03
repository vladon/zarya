#include "dns/DnsGeoUtils.h"

namespace zarya {

bool DnsGeoUtils::valueReferencesGeoData(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    const QString lower = trimmed.toLower();
    return lower.startsWith(QStringLiteral("geoip:"))
           || lower.startsWith(QStringLiteral("geosite:"));
}

QStringList DnsGeoUtils::geoReferencesUsed(const DnsProfile& profile)
{
    QStringList references;
    auto appendUnique = [&references](const QString& value) {
        if (valueReferencesGeoData(value) && !references.contains(value)) {
            references.append(value);
        }
    };

    for (const DnsServer& server : profile.servers) {
        if (!server.enabled) {
            continue;
        }
        for (const QString& domain : server.domains) {
            appendUnique(domain);
        }
        for (const QString& expectIp : server.expectIPs) {
            appendUnique(expectIp);
        }
    }
    return references;
}

bool DnsGeoUtils::profileUsesGeoData(const DnsProfile& profile)
{
    return !geoReferencesUsed(profile).isEmpty();
}

} // namespace zarya
