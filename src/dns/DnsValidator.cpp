#include "dns/DnsValidator.h"

#include "dns/XrayDnsGenerator.h"
#include "domain/DnsProfileMode.h"

#include <QRegularExpression>

namespace zarya {

namespace {

bool looksLikeIpOrHost(const QString& address)
{
    const QString trimmed = address.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (trimmed.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        return true;
    }
    if (trimmed == QStringLiteral("localhost")) {
        return true;
    }
    const QString hostPart = trimmed.section(QLatin1Char(':'), 0, 0);
    static const QRegularExpression ipv4Pattern(
        QStringLiteral(R"(^(\d{1,3}\.){3}\d{1,3}$)"));
    return ipv4Pattern.match(hostPart).hasMatch() || hostPart.contains(QLatin1Char('.'));
}

} // namespace

QStringList DnsValidator::warnings(const DnsProfile& profile)
{
    QStringList warnings;
    if (profile.name.trimmed().isEmpty()) {
        warnings.append(QStringLiteral("DNS profile name is empty."));
    }

    const XrayDnsGenerator generator;
    if (profile.mode != DnsProfileMode::System && profile.enabled
        && generator.enabledServerCount(profile) == 0) {
        warnings.append(QStringLiteral("Enabled DNS profile has no servers configured."));
    }

    for (const DnsServer& server : profile.servers) {
        if (!server.enabled) {
            continue;
        }
        const QString address = server.address.trimmed();
        if (address.isEmpty()) {
            warnings.append(QStringLiteral("A DNS server has an empty address."));
            continue;
        }
        if (server.kind == DnsServerKind::DoH
            && !address.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
            warnings.append(
                QStringLiteral("DoH server address should start with https://: %1").arg(address));
        }
        if (server.kind == DnsServerKind::PlainIp && !looksLikeIpOrHost(address)) {
            warnings.append(QStringLiteral("Plain DNS server address may be invalid: %1")
                                .arg(address));
        }
        for (const QString& domain : server.domains) {
            const QString trimmed = domain.trimmed();
            if (trimmed.isEmpty()) {
                continue;
            }
            const QString lower = trimmed.toLower();
            if (!(lower.startsWith(QStringLiteral("geosite:"))
                  || lower.startsWith(QStringLiteral("domain:"))
                  || lower.startsWith(QStringLiteral("full:"))
                  || lower.startsWith(QStringLiteral("regexp:"))
                  || lower.startsWith(QStringLiteral("keyword:")))) {
                warnings.append(QStringLiteral("Unusual domain matcher: %1").arg(trimmed));
            }
        }
        for (const QString& expectIp : server.expectIPs) {
            const QString trimmed = expectIp.trimmed();
            if (trimmed.isEmpty()) {
                continue;
            }
            if (!trimmed.startsWith(QStringLiteral("geoip:"), Qt::CaseInsensitive)
                && !trimmed.contains(QLatin1Char('.')) && !trimmed.contains(QLatin1Char(':'))) {
                warnings.append(QStringLiteral("Unusual expectIPs value: %1").arg(trimmed));
            }
        }
    }

    return warnings;
}

QStringList DnsValidator::interactionWarnings(const DnsProfile& dnsProfile,
                                              const QString& routingDomainStrategy,
                                              bool routingUsesGeoData)
{
    QStringList warnings;
    if (routingUsesGeoData && dnsProfile.mode == DnsProfileMode::System) {
        warnings.append(QStringLiteral(
            "The active routing profile uses geoip/geosite rules, but DNS profile is System DNS. "
            "Domain-to-IP routing may depend on system resolution behavior."));
    }

    if (dnsProfile.mode != DnsProfileMode::System
        && routingDomainStrategy.trimmed().compare(QStringLiteral("AsIs"), Qt::CaseInsensitive)
               == 0) {
        warnings.append(QStringLiteral(
            "Routing domainStrategy is AsIs. Xray may not resolve domains for IP-based routing "
            "unless required by rules."));
    }

    return warnings;
}

} // namespace zarya
