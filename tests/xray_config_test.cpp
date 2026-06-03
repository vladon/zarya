#include "core/XrayAdapter.h"
#include "core/XrayConfigTestHelpers.h"
#include "core/XrayVlessGenerator.h"
#include "domain/ProtocolType.h"
#include "domain/RoutingMode.h"
#include "domain/DnsProfile.h"
#include "domain/DnsProfileMode.h"
#include "domain/RoutingProfile.h"
#include "dns/XrayDnsGenerator.h"
#include "routing/XrayRoutingGenerator.h"
#include "import/VlessUriParser.h"
#include "storage/ProfileStore.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include <cstdio>

namespace {

bool fail(const char* message)
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    return false;
}

bool pass(const char* message)
{
    std::fprintf(stdout, "PASS: %s\n", message);
    return true;
}

QJsonObject loadJsonObject(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return {};
    }
    return document.object();
}

QJsonObject firstProxyOutbound(const QJsonObject& config)
{
    const QJsonArray outbounds = config.value(QStringLiteral("outbounds")).toArray();
    if (outbounds.isEmpty()) {
        return {};
    }
    return outbounds.first().toObject();
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    if (argc >= 3 && QString::fromUtf8(argv[1]) == QStringLiteral("--write-config")) {
        const zarya::Profile sample = zarya::testhelpers::sampleVlessRealityProfile();
        const zarya::ConfigGenerationResult generated = zarya::XrayVlessGenerator::generate(sample);
        if (!generated.success) {
            std::fprintf(stderr, "FAIL: %s\n", generated.errorMessage.toUtf8().constData());
            return 1;
        }
        QFile file(QString::fromUtf8(argv[2]));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            std::fprintf(stderr, "FAIL: %s\n", file.errorString().toUtf8().constData());
            return 1;
        }
        file.write(QJsonDocument(generated.config).toJson(QJsonDocument::Indented));
        std::fprintf(stdout, "Wrote config to %s\n", argv[2]);
        return 0;
    }

    const QString examplesDir = QStringLiteral(ZARYA_EXAMPLES_DIR);
    bool ok = true;

    const zarya::Profile sample = zarya::testhelpers::sampleVlessRealityProfile();
    QString generateError;
    const zarya::ConfigGenerationResult generated = zarya::XrayVlessGenerator::generate(sample);
    if (!generated.success) {
        ok &= fail(generated.errorMessage.toUtf8().constData());
    } else {
        ok &= pass("Sample REALITY profile generates config");
    }

    const QJsonObject proxyOutbound = firstProxyOutbound(generated.config);
    if (!zarya::testhelpers::proxyOutboundHasReality(proxyOutbound)) {
        ok &= fail("Proxy outbound missing REALITY stream settings");
    } else {
        ok &= pass("Proxy outbound contains REALITY");
    }

    const QJsonObject reality =
        proxyOutbound.value(QStringLiteral("streamSettings"))
            .toObject()
            .value(QStringLiteral("realitySettings"))
            .toObject();
    QString missingKey;
    const QStringList realityKeys = {QStringLiteral("publicKey"), QStringLiteral("serverName"),
                                     QStringLiteral("shortId"), QStringLiteral("fingerprint"),
                                     QStringLiteral("spiderX")};
    if (!zarya::testhelpers::jsonContainsKeys(reality, realityKeys, &missingKey)) {
        ok &= fail(QStringLiteral("Missing reality key: %1").arg(missingKey).toUtf8().constData());
    } else {
        ok &= pass("REALITY settings include required client keys");
    }

    const QJsonObject users =
        proxyOutbound.value(QStringLiteral("settings"))
            .toObject()
            .value(QStringLiteral("vnext"))
            .toArray()
            .first()
            .toObject()
            .value(QStringLiteral("users"))
            .toArray()
            .first()
            .toObject();
    if (users.value(QStringLiteral("flow")).toString()
        != QStringLiteral("xtls-rprx-vision")) {
        ok &= fail("VLESS user flow is not xtls-rprx-vision");
    } else {
        ok &= pass("VLESS user flow is xtls-rprx-vision");
    }

    QString fileError;
    const QJsonObject expectedConfig =
        loadJsonObject(examplesDir + QStringLiteral("/xray-vless-reality.sample.json"), &fileError);
    if (expectedConfig.isEmpty()) {
        ok &= fail(fileError.toUtf8().constData());
    } else {
        const QJsonObject expectedProxy = firstProxyOutbound(expectedConfig);
        const QJsonObject expectedReality =
            expectedProxy.value(QStringLiteral("streamSettings"))
                .toObject()
                .value(QStringLiteral("realitySettings"))
                .toObject();
        if (reality.value(QStringLiteral("publicKey")) != expectedReality.value(QStringLiteral("publicKey"))
            || reality.value(QStringLiteral("serverName"))
                   != expectedReality.value(QStringLiteral("serverName"))) {
            ok &= fail("Generated REALITY settings differ from example file");
        } else {
            ok &= pass("Generated REALITY settings match example publicKey and serverName");
        }
    }

    const QString vlessLink =
        QStringLiteral("vless://11111111-1111-1111-1111-111111111111@host.example.com:443?type=tcp"
                       "&security=reality&pbk=yWrHCV6C0UYNw6nzM0rhDlIUjfLlt28A9h8SkqR52V0&fp=chrome"
                       "&sni=example.com&sid=a1b2c3d4&spx=%2F&flow=xtls-rprx-vision#Test%20Reality");
    const zarya::VlessParseResult parsed = zarya::VlessUriParser::parseLink(vlessLink);
    if (!parsed.success) {
        ok &= fail(parsed.errorMessage.toUtf8().constData());
    } else if (parsed.profile.name != QStringLiteral("Test Reality")
               || parsed.profile.address != QStringLiteral("host.example.com")
               || parsed.profile.port != 443
               || !parsed.profile.isSecurityReality()) {
        ok &= fail("VLESS import link parsed with unexpected profile fields");
    } else {
        ok &= pass("VLESS import link parses into REALITY profile");
    }

    zarya::XrayAdapter adapter;
    const zarya::Profile vmessSample = zarya::testhelpers::sampleVmessTcpTlsProfile();
    if (!adapter.supportsProfile(vmessSample)) {
        ok &= fail("VMess TCP TLS profile should be supported");
    } else {
        ok &= pass("VMess TCP TLS profile is supported");
        const auto vmessGenerated = adapter.generateConfig(vmessSample);
        if (!vmessGenerated.success) {
            ok &= fail(vmessGenerated.errorMessage.toUtf8().constData());
        } else {
            const QJsonObject vmessOutbound = firstProxyOutbound(vmessGenerated.config);
            if (vmessOutbound.value(QStringLiteral("protocol")).toString()
                != QStringLiteral("vmess")) {
                ok &= fail("VMess outbound protocol mismatch");
            } else {
                ok &= pass("VMess TCP TLS generates vmess outbound");
            }
        }
    }

    const zarya::Profile trojanSample = zarya::testhelpers::sampleTrojanTlsProfile();
    const auto trojanGenerated = adapter.generateConfig(trojanSample);
    if (!trojanGenerated.success) {
        ok &= fail(trojanGenerated.errorMessage.toUtf8().constData());
    } else {
        const QJsonObject trojanOutbound = firstProxyOutbound(trojanGenerated.config);
        const QString password =
            trojanOutbound.value(QStringLiteral("settings"))
                .toObject()
                .value(QStringLiteral("servers"))
                .toArray()
                .first()
                .toObject()
                .value(QStringLiteral("password"))
                .toString();
        if (password != QStringLiteral("secret")) {
            ok &= fail("Trojan outbound password mismatch");
        } else {
            ok &= pass("Trojan TLS generates trojan outbound");
        }
    }

    const zarya::Profile ssSample = zarya::testhelpers::sampleShadowsocksProfile();
    const auto ssGenerated = adapter.generateConfig(ssSample);
    if (!ssGenerated.success) {
        ok &= fail(ssGenerated.errorMessage.toUtf8().constData());
    } else {
        const QJsonObject ssOutbound = firstProxyOutbound(ssGenerated.config);
        const QJsonObject server =
            ssOutbound.value(QStringLiteral("settings"))
                .toObject()
                .value(QStringLiteral("servers"))
                .toArray()
                .first()
                .toObject();
        if (server.value(QStringLiteral("method")).toString()
            != QStringLiteral("2022-blake3-aes-128-gcm")) {
            ok &= fail("Shadowsocks method not preserved");
        } else {
            ok &= pass("Shadowsocks outbound preserves method");
        }
    }

    const QVector<zarya::RoutingProfile> builtInProfiles =
        zarya::RoutingProfile::createBuiltInProfiles();
    zarya::RoutingProfile bypassLan;
    for (const zarya::RoutingProfile& profile : builtInProfiles) {
        if (profile.id == zarya::RoutingBuiltinIds::bypassLan()) {
            bypassLan = profile;
            break;
        }
    }
    if (bypassLan.id.isEmpty()) {
        ok &= fail("Built-in Bypass LAN routing profile missing");
    } else {
        const zarya::XrayRoutingGenerator routingGenerator;
        const QJsonObject bypassRouting = routingGenerator.generate(bypassLan);
        const QJsonArray rules = bypassRouting.value(QStringLiteral("rules")).toArray();
        bool hasDomainPrivate = false;
        bool hasIpPrivate = false;
        for (const QJsonValue& value : rules) {
            const QJsonObject rule = value.toObject();
            if (rule.value(QStringLiteral("outboundTag")).toString()
                != QStringLiteral("direct")) {
                continue;
            }
            const QJsonArray domains = rule.value(QStringLiteral("domain")).toArray();
            for (const QJsonValue& domain : domains) {
                if (domain.toString() == QStringLiteral("geosite:private")) {
                    hasDomainPrivate = true;
                }
            }
            const QJsonArray ips = rule.value(QStringLiteral("ip")).toArray();
            for (const QJsonValue& ip : ips) {
                if (ip.toString() == QStringLiteral("geoip:private")) {
                    hasIpPrivate = true;
                }
            }
        }
        if (!hasDomainPrivate || !hasIpPrivate) {
            ok &= fail("Bypass LAN routing missing geosite/geoip private direct rules");
        } else {
            ok &= pass("Bypass LAN routing generates private direct rules");
        }

        zarya::XrayInboundPorts ports;
        ports.socksPort = 10808;
        ports.httpPort = 10809;
        const auto routedConfig = adapter.generateConfig(vmessSample, ports, bypassLan);
        if (!routedConfig.success) {
            ok &= fail("Config with routing profile failed to generate");
        } else {
            const QJsonObject routing =
                routedConfig.config.value(QStringLiteral("routing")).toObject();
            if (routing.value(QStringLiteral("domainStrategy")).toString()
                != QStringLiteral("AsIs")) {
                ok &= fail("Routing domainStrategy should default to AsIs");
            } else {
                ok &= pass("Routed config uses AsIs domain strategy");
            }
        }
    }

    const QVector<zarya::DnsProfile> dnsBuiltIns = zarya::DnsProfile::createBuiltInProfiles();
    zarya::DnsProfile secureRemoteDns;
    for (const zarya::DnsProfile& profile : dnsBuiltIns) {
        if (profile.id == zarya::DnsBuiltinIds::secureRemote()) {
            secureRemoteDns = profile;
            break;
        }
    }
    if (secureRemoteDns.id.isEmpty()) {
        ok &= fail("Built-in Secure Remote DNS profile missing");
    } else {
        const zarya::XrayDnsGenerator dnsGenerator;
        if (dnsGenerator.shouldGenerateDnsObject(zarya::DnsProfile::builtInSystemDns())) {
            ok &= fail("System DNS should not generate dns object");
        } else {
            ok &= pass("System DNS omits Xray dns object");
        }

        const QJsonObject secureDnsJson = dnsGenerator.generate(secureRemoteDns);
        const QJsonArray secureServers = secureDnsJson.value(QStringLiteral("servers")).toArray();
        if (secureServers.size() < 2) {
            ok &= fail("Secure Remote DNS should include multiple servers");
        } else if (secureDnsJson.value(QStringLiteral("queryStrategy")).toString()
                   != QStringLiteral("UseIP")) {
            ok &= fail("Secure Remote DNS queryStrategy should be UseIP");
        } else {
            ok &= pass("Secure Remote DNS generates valid dns JSON");
        }

        zarya::XrayInboundPorts ports;
        ports.socksPort = 10808;
        ports.httpPort = 10809;
        const auto dnsConfig = adapter.generateConfig(
            vmessSample, ports, zarya::RoutingProfile::builtInProxyAll(), secureRemoteDns);
        if (!dnsConfig.success) {
            ok &= fail("Config with DNS profile failed to generate");
        } else if (!dnsConfig.config.contains(QStringLiteral("dns"))) {
            ok &= fail("Full config should include dns section for Secure Remote DNS");
        } else {
            ok &= pass("Full Xray config includes dns object when DNS profile requires it");
        }
    }

    zarya::ProfileStore store(examplesDir + QStringLiteral("/profiles-vless-reality.sample.json"));
    const QVector<zarya::Profile> loaded = store.load(&fileError);
    if (loaded.isEmpty()) {
        ok &= fail("Failed to load sample profiles file");
    } else {
        ok &= pass("Backward-compatible profile JSON loads");
        const auto loadedResult = zarya::XrayVlessGenerator::generate(loaded.first());
        if (!loadedResult.success) {
            ok &= fail("Loaded sample profile failed Xray generation");
        } else {
            ok &= pass("Loaded sample profile generates Xray config");
        }
    }

    return ok ? 0 : 1;
}
