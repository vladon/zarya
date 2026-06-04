#include "domain/DnsProfile.h"
#include "domain/DnsProfileMode.h"
#include "domain/RoutingMode.h"
#include "domain/RoutingProfile.h"
#include "runtime/singbox/SingBoxDnsGenerator.h"
#include "runtime/singbox/SingBoxRouteGenerator.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>

namespace {

int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    } else {
        std::printf("PASS: %s\n", message);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const zarya::SingBoxRouteGenerator routeGenerator;
    const zarya::SingBoxDnsGenerator dnsGenerator;

    {
        const QJsonObject route = routeGenerator.generateRoute(
            zarya::RoutingProfile::builtInProxyAll(), {});
        check(route.value(QStringLiteral("final")).toString() == QStringLiteral("proxy"),
              "Proxy All maps to sing-box final proxy");
    }

    {
        QVector<zarya::RoutingProfile> builtIns = zarya::RoutingProfile::createBuiltInProfiles();
        zarya::RoutingProfile bypassLan;
        for (const zarya::RoutingProfile& profile : builtIns) {
            if (profile.mode == zarya::RoutingMode::BypassLan) {
                bypassLan = profile;
                break;
            }
        }
        const QJsonObject route = routeGenerator.generateRoute(bypassLan, {});
        const QJsonArray rules = route.value(QStringLiteral("rules")).toArray();
        bool hasPrivate = false;
        for (const QJsonValue& value : rules) {
            const QJsonObject rule = value.toObject();
            if (rule.value(QStringLiteral("ip_is_private")).toBool()) {
                hasPrivate = true;
            }
        }
        check(hasPrivate, "Bypass LAN includes ip_is_private direct rule");
    }

    {
        QVector<zarya::RoutingProfile> builtIns = zarya::RoutingProfile::createBuiltInProfiles();
        zarya::RoutingProfile bypassRu;
        for (const zarya::RoutingProfile& profile : builtIns) {
            if (profile.mode == zarya::RoutingMode::BypassRu) {
                bypassRu = profile;
                break;
            }
        }
        const QJsonObject route = routeGenerator.generateRoute(bypassRu, {});
        const QJsonArray rules = route.value(QStringLiteral("rules")).toArray();
        bool hasGeositeRu = false;
        bool hasGeoipRu = false;
        for (const QJsonValue& value : rules) {
            const QJsonObject rule = value.toObject();
            const QJsonArray geosite = rule.value(QStringLiteral("geosite")).toArray();
            for (const QJsonValue& tag : geosite) {
                if (tag.toString() == QStringLiteral("ru")) {
                    hasGeositeRu = true;
                }
            }
            const QJsonArray geoip = rule.value(QStringLiteral("geoip")).toArray();
            for (const QJsonValue& tag : geoip) {
                if (tag.toString() == QStringLiteral("ru")) {
                    hasGeoipRu = true;
                }
            }
        }
        check(hasGeositeRu && hasGeoipRu, "Bypass RU includes geosite and geoip direct rules");
    }

    {
        zarya::SingBoxDnsGenerationOptions options;
        const QJsonObject dns =
            dnsGenerator.generateDns(zarya::DnsProfile::builtInSystemDns(), options);
        check(dns.value(QStringLiteral("final")).toString() == QStringLiteral("local"),
              "System DNS uses local final in TUN");
    }

    {
        QVector<zarya::DnsProfile> builtIns = zarya::DnsProfile::createBuiltInProfiles();
        zarya::DnsProfile secureRemote;
        for (const zarya::DnsProfile& profile : builtIns) {
            if (profile.mode == zarya::DnsProfileMode::SecureRemote) {
                secureRemote = profile;
                break;
            }
        }
        const QJsonObject dns = dnsGenerator.generateDns(secureRemote, {});
        const QJsonArray servers = dns.value(QStringLiteral("servers")).toArray();
        bool foundRemote = false;
        for (const QJsonValue& value : servers) {
            const QJsonObject server = value.toObject();
            if (server.value(QStringLiteral("tag")).toString() == QStringLiteral("remote-1")) {
                foundRemote = server.value(QStringLiteral("address")).toString().contains(
                    QStringLiteral("cloudflare-dns.com"));
                check(server.value(QStringLiteral("detour")).toString() == QStringLiteral("proxy"),
                      "Secure Remote DNS DoH detours via proxy");
            }
        }
        check(foundRemote, "Secure Remote DNS includes Cloudflare DoH remote-1");
    }

    return failures == 0 ? 0 : 1;
}
