#include "runtime/singbox/SingBoxDnsGenerator.h"

#include "runtime/singbox/SingBoxRuleSetManager.h"

#include <QJsonArray>

namespace zarya {

namespace {

void appendWarning(QStringList* warnings, const QString& message)
{
    if (warnings) {
        warnings->append(message);
    }
}

QString detourForServer(const DnsServer& server)
{
    if (server.kind == DnsServerKind::DoH) {
        return QStringLiteral("proxy");
    }
    if (server.kind == DnsServerKind::Local) {
        return QStringLiteral("direct");
    }
    return QStringLiteral("direct");
}

QString makeServerTag(int index, const DnsServer& server)
{
    if (!server.tag.trimmed().isEmpty()) {
        return server.tag.trimmed();
    }
    return QStringLiteral("dns-%1").arg(index);
}

QJsonObject serverObject(const DnsServer& server, const QString& tag, QStringList* warnings)
{
    QString address = server.address.trimmed();
    if (address.isEmpty()) {
        return {};
    }
    if (server.port > 0 && !address.contains(QLatin1Char(':'))
        && server.kind == DnsServerKind::PlainIp) {
        address += QStringLiteral(":%1").arg(server.port);
    }

    QJsonObject object;
    object.insert(QStringLiteral("tag"), tag);
    object.insert(QStringLiteral("address"), address);
    const QString detour = detourForServer(server);
    if (!detour.isEmpty()) {
        object.insert(QStringLiteral("detour"), detour);
    }

    if (!server.expectIPs.isEmpty()) {
        appendWarning(warnings,
                      QStringLiteral("expectIPs is not mapped to sing-box DNS for server %1.")
                          .arg(tag));
    }
    return object;
}

QJsonArray translateDomainMatchers(const QStringList& domains)
{
    QJsonArray matchers;
    for (const QString& domain : domains) {
        const QString trimmed = domain.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (trimmed.startsWith(QStringLiteral("geosite:"), Qt::CaseInsensitive)) {
            matchers.append(QJsonObject{
                {QStringLiteral("geosite"), QJsonArray{trimmed.mid(8).trimmed()}}});
        } else if (trimmed.startsWith(QStringLiteral("domain:"), Qt::CaseInsensitive)) {
            matchers.append(QJsonObject{
                {QStringLiteral("domain_suffix"), QJsonArray{trimmed.mid(7).trimmed()}}});
        } else if (trimmed.startsWith(QStringLiteral("full:"), Qt::CaseInsensitive)) {
            matchers.append(QJsonObject{
                {QStringLiteral("domain"), QJsonArray{trimmed.mid(5).trimmed()}}});
        } else {
            matchers.append(QJsonObject{
                {QStringLiteral("domain_suffix"), QJsonArray{trimmed}}});
        }
    }
    return matchers;
}

} // namespace

QJsonObject SingBoxDnsGenerator::generateDns(const DnsProfile& dnsProfile,
                                             const SingBoxDnsGenerationOptions& options,
                                             QStringList* warnings) const
{
    Q_UNUSED(options);

    if (!dnsProfile.enabled) {
        appendWarning(warnings, QStringLiteral("DNS profile is disabled."));
        return {};
    }

    switch (dnsProfile.mode) {
    case DnsProfileMode::System: {
        appendWarning(warnings,
                      QStringLiteral(
                          "System DNS in TUN mode may use OS DNS and may not prevent DNS leaks."));
        QJsonObject localServer;
        localServer.insert(QStringLiteral("tag"), QStringLiteral("local"));
        localServer.insert(QStringLiteral("address"), QStringLiteral("local"));
        QJsonObject dns;
        dns.insert(QStringLiteral("servers"), QJsonArray{localServer});
        dns.insert(QStringLiteral("final"), QStringLiteral("local"));
        return dns;
    }
    case DnsProfileMode::SecureRemote: {
        QJsonObject remote1;
        remote1.insert(QStringLiteral("tag"), QStringLiteral("remote-1"));
        remote1.insert(QStringLiteral("address"),
                       QStringLiteral("https://cloudflare-dns.com/dns-query"));
        remote1.insert(QStringLiteral("detour"), QStringLiteral("proxy"));

        QJsonObject remote2;
        remote2.insert(QStringLiteral("tag"), QStringLiteral("remote-2"));
        remote2.insert(QStringLiteral("address"), QStringLiteral("https://dns.google/dns-query"));
        remote2.insert(QStringLiteral("detour"), QStringLiteral("proxy"));

        QJsonObject dns;
        dns.insert(QStringLiteral("servers"), QJsonArray{remote1, remote2});
        dns.insert(QStringLiteral("final"), QStringLiteral("remote-1"));
        return dns;
    }
    case DnsProfileMode::ChinaDirectGlobalRemote: {
        QJsonObject localCn;
        localCn.insert(QStringLiteral("tag"), QStringLiteral("local-cn"));
        localCn.insert(QStringLiteral("address"), QStringLiteral("223.5.5.5"));
        localCn.insert(QStringLiteral("detour"), QStringLiteral("direct"));

        QJsonObject remote;
        remote.insert(QStringLiteral("tag"), QStringLiteral("remote"));
        remote.insert(QStringLiteral("address"),
                      QStringLiteral("https://cloudflare-dns.com/dns-query"));
        remote.insert(QStringLiteral("detour"), QStringLiteral("proxy"));

        QJsonObject cnRule;
        cnRule.insert(QStringLiteral("geosite"), QJsonArray{QStringLiteral("cn")});
        cnRule.insert(QStringLiteral("server"), QStringLiteral("local-cn"));

        QJsonObject globalRule;
        globalRule.insert(QStringLiteral("geosite"),
                          QJsonArray{QStringLiteral("geolocation-!cn")});
        globalRule.insert(QStringLiteral("server"), QStringLiteral("remote"));

        QJsonObject dns;
        dns.insert(QStringLiteral("servers"), QJsonArray{localCn, remote});
        dns.insert(QStringLiteral("rules"), QJsonArray{cnRule, globalRule});
        dns.insert(QStringLiteral("final"), QStringLiteral("remote"));

        QStringList geoTags = {QStringLiteral("cn"), QStringLiteral("geolocation-!cn")};
        SingBoxRuleSetManager::appendGeoCompatibilityWarnings(geoTags, warnings);
        return dns;
    }
    case DnsProfileMode::Custom: {
        QJsonArray servers;
        QJsonArray rules;
        QString finalTag;
        int index = 0;

        for (const DnsServer& server : dnsProfile.servers) {
            if (!server.enabled || server.kind == DnsServerKind::FakeDnsPlaceholder) {
                continue;
            }
            const QString tag = makeServerTag(++index, server);
            const QJsonObject serverJson = serverObject(server, tag, warnings);
            if (serverJson.isEmpty()) {
                continue;
            }
            servers.append(serverJson);
            if (finalTag.isEmpty()) {
                finalTag = tag;
            }

            if (!server.domains.isEmpty()) {
                QJsonObject rule;
                const QJsonArray domainMatchers = translateDomainMatchers(server.domains);
                if (!domainMatchers.isEmpty()) {
                    if (domainMatchers.size() == 1) {
                        const QJsonObject matcher = domainMatchers.first().toObject();
                        for (auto it = matcher.begin(); it != matcher.end(); ++it) {
                            rule.insert(it.key(), it.value());
                        }
                    } else {
                        appendWarning(warnings,
                                      QStringLiteral(
                                          "Multiple domain matchers on DNS server %1 were "
                                          "simplified to the first matcher only.")
                                          .arg(tag));
                        const QJsonObject matcher = domainMatchers.first().toObject();
                        for (auto it = matcher.begin(); it != matcher.end(); ++it) {
                            rule.insert(it.key(), it.value());
                        }
                    }
                    rule.insert(QStringLiteral("server"), tag);
                    rules.append(rule);
                }
            }
        }

        if (servers.isEmpty()) {
            appendWarning(warnings, QStringLiteral("Custom DNS profile has no enabled servers."));
            return {};
        }

        QJsonObject dns;
        dns.insert(QStringLiteral("servers"), servers);
        if (!rules.isEmpty()) {
            dns.insert(QStringLiteral("rules"), rules);
        }
        dns.insert(QStringLiteral("final"), finalTag.isEmpty() ? QStringLiteral("dns-1") : finalTag);
        return dns;
    }
    }

    appendWarning(warnings, QStringLiteral("Unsupported DNS profile mode."));
    return {};
}

} // namespace zarya
