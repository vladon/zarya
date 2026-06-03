#include "domain/DnsProfile.h"

#include <QUuid>

namespace zarya {

namespace {

DnsServer makeServer(const QString& address, DnsServerKind kind = DnsServerKind::PlainIp,
                     const QStringList& domains = {}, const QStringList& expectIPs = {})
{
    DnsServer server = DnsServer::createDefault();
    server.address = address;
    server.kind = kind;
    server.domains = domains;
    server.expectIPs = expectIPs;
    if (address.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        server.kind = DnsServerKind::DoH;
    }
    return server;
}

DnsProfile makeBuiltIn(const QString& id, const QString& name, DnsProfileMode mode,
                       DnsQueryStrategy queryStrategy, const QVector<DnsServer>& servers)
{
    DnsProfile profile;
    profile.id = id;
    profile.name = name;
    profile.mode = mode;
    profile.enabled = true;
    profile.isBuiltIn = true;
    profile.queryStrategy = queryStrategy;
    profile.servers = servers;
    profile.createdAt = QDateTime::currentDateTimeUtc();
    profile.updatedAt = profile.createdAt;
    return profile;
}

} // namespace

DnsProfile DnsProfile::builtInSystemDns()
{
    return makeBuiltIn(DnsBuiltinIds::systemDns(), QStringLiteral("System DNS"),
                       DnsProfileMode::System, DnsQueryStrategy::UseSystemDefault, {});
}

QVector<DnsProfile> DnsProfile::createBuiltInProfiles()
{
    QVector<DnsProfile> profiles;
    profiles.append(builtInSystemDns());
    profiles.append(makeBuiltIn(
        DnsBuiltinIds::secureRemote(), QStringLiteral("Secure Remote DNS"),
        DnsProfileMode::SecureRemote, DnsQueryStrategy::UseIP,
        {makeServer(QStringLiteral("https://cloudflare-dns.com/dns-query"), DnsServerKind::DoH),
         makeServer(QStringLiteral("https://dns.google/dns-query"), DnsServerKind::DoH)}));
    profiles.append(makeBuiltIn(
        DnsBuiltinIds::chinaDirectGlobalRemote(),
        QStringLiteral("China Direct / Global Remote"), DnsProfileMode::ChinaDirectGlobalRemote,
        DnsQueryStrategy::UseIP,
        {makeServer(QStringLiteral("223.5.5.5"), DnsServerKind::PlainIp,
                    {QStringLiteral("geosite:cn")}, {QStringLiteral("geoip:cn")}),
         makeServer(QStringLiteral("https://cloudflare-dns.com/dns-query"), DnsServerKind::DoH,
                    {QStringLiteral("geosite:geolocation-!cn")}),
         makeServer(QStringLiteral("https://dns.google/dns-query"), DnsServerKind::DoH)}));
    profiles.append(makeBuiltIn(DnsBuiltinIds::customTemplate(), QStringLiteral("Custom"),
                                  DnsProfileMode::Custom, DnsQueryStrategy::UseSystemDefault, {}));
    return profiles;
}

} // namespace zarya
