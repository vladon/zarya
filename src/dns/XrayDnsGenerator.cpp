#include "dns/XrayDnsGenerator.h"

#include "domain/DnsQueryStrategy.h"

#include <QJsonArray>

namespace zarya {

namespace {

QString effectiveServerAddress(const DnsServer& server)
{
    QString address = server.address.trimmed();
    if (server.port > 0 && !address.contains(QLatin1Char(':'))
        && server.kind == DnsServerKind::PlainIp) {
        address += QStringLiteral(":%1").arg(server.port);
    }
    return address;
}

bool isSimpleServerObject(const DnsServer& server)
{
    return server.domains.isEmpty() && server.expectIPs.isEmpty() && server.tag.trimmed().isEmpty()
           && server.timeoutMs <= 0 && !server.skipFallback
           && server.queryStrategy.trimmed().isEmpty();
}

QJsonValue serverToJson(const DnsServer& server)
{
    const QString address = effectiveServerAddress(server);
    if (address.isEmpty()) {
        return {};
    }

    if (isSimpleServerObject(server)) {
        return address;
    }

    QJsonObject object;
    object.insert(QStringLiteral("address"), address);
    if (!server.domains.isEmpty()) {
        QJsonArray domains;
        for (const QString& domain : server.domains) {
            const QString trimmed = domain.trimmed();
            if (!trimmed.isEmpty()) {
                domains.append(trimmed);
            }
        }
        if (!domains.isEmpty()) {
            object.insert(QStringLiteral("domains"), domains);
        }
    }
    if (!server.expectIPs.isEmpty()) {
        QJsonArray expectIPs;
        for (const QString& expectIp : server.expectIPs) {
            const QString trimmed = expectIp.trimmed();
            if (!trimmed.isEmpty()) {
                expectIPs.append(trimmed);
            }
        }
        if (!expectIPs.isEmpty()) {
            object.insert(QStringLiteral("expectIPs"), expectIPs);
        }
    }
    if (!server.tag.trimmed().isEmpty()) {
        object.insert(QStringLiteral("tag"), server.tag.trimmed());
    }
    if (server.timeoutMs > 0) {
        object.insert(QStringLiteral("timeoutMs"), server.timeoutMs);
    }
    if (server.skipFallback) {
        object.insert(QStringLiteral("skipFallback"), true);
    }
    const QString serverStrategy = server.queryStrategy.trimmed();
    if (!serverStrategy.isEmpty()) {
        object.insert(QStringLiteral("queryStrategy"), serverStrategy);
    }
    return object;
}

} // namespace

bool XrayDnsGenerator::shouldGenerateDnsObject(const DnsProfile& profile) const
{
    if (!profile.enabled) {
        return false;
    }
    if (profile.mode == DnsProfileMode::System) {
        return false;
    }
    return enabledServerCount(profile) > 0;
}

int XrayDnsGenerator::enabledServerCount(const DnsProfile& profile) const
{
    int count = 0;
    for (const DnsServer& server : profile.servers) {
        if (server.enabled && !server.address.trimmed().isEmpty()
            && server.kind != DnsServerKind::FakeDnsPlaceholder) {
            ++count;
        }
    }
    return count;
}

QJsonObject XrayDnsGenerator::generate(const DnsProfile& profile) const
{
    if (!shouldGenerateDnsObject(profile)) {
        return {};
    }

    QJsonObject dns;

    if (!profile.hosts.isEmpty()) {
        QJsonObject hosts;
        for (auto it = profile.hosts.constBegin(); it != profile.hosts.constEnd(); ++it) {
            const QString key = it.key().trimmed();
            const QString value = it.value().trimmed();
            if (!key.isEmpty() && !value.isEmpty()) {
                hosts.insert(key, value);
            }
        }
        if (!hosts.isEmpty()) {
            dns.insert(QStringLiteral("hosts"), hosts);
        }
    }

    QJsonArray servers;
    for (const DnsServer& server : profile.servers) {
        if (!server.enabled || server.kind == DnsServerKind::FakeDnsPlaceholder) {
            continue;
        }
        const QJsonValue value = serverToJson(server);
        if (value.isString() && !value.toString().isEmpty()) {
            servers.append(value.toString());
        } else if (value.isObject() && !value.toObject().isEmpty()) {
            servers.append(value.toObject());
        }
    }
    dns.insert(QStringLiteral("servers"), servers);

    const QString queryStrategy = dnsQueryStrategyToXrayValue(profile.queryStrategy);
    if (!queryStrategy.isEmpty()) {
        dns.insert(QStringLiteral("queryStrategy"), queryStrategy);
    }

    if (profile.disableCache) {
        dns.insert(QStringLiteral("disableCache"), true);
    }
    if (profile.disableFallback) {
        dns.insert(QStringLiteral("disableFallback"), true);
    }
    if (profile.disableFallbackIfMatch) {
        dns.insert(QStringLiteral("disableFallbackIfMatch"), true);
    }

    return dns;
}

} // namespace zarya
