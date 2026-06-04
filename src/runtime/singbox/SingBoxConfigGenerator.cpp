#include "runtime/singbox/SingBoxConfigGenerator.h"

#include "runtime/singbox/SingBoxDnsGenerator.h"
#include "runtime/singbox/SingBoxRouteGenerator.h"

#include <QJsonArray>

namespace zarya {

namespace {

void mergeWarnings(SingBoxConfigGenerationResult& result, const QStringList& warnings)
{
    for (const QString& warning : warnings) {
        if (!result.warnings.contains(warning)) {
            result.warnings.append(warning);
        }
    }
}

} // namespace

QList<ConfigWarning> SingBoxConfigGenerator::classifyWarnings(const QStringList& warnings) const
{
    QList<ConfigWarning> classified;
    for (const QString& warning : warnings) {
        const QString lower = warning.toLower();
        if (lower.contains(QStringLiteral("not supported by experimental tun"))
            || lower.contains(QStringLiteral("cannot be represented"))) {
            classified.append(ConfigWarning::blocking(warning));
        } else if (lower.contains(QStringLiteral("dns leak"))
                   || lower.contains(QStringLiteral("rule-set"))
                   || lower.contains(QStringLiteral("dns hijack"))
                   || lower.contains(QStringLiteral("not supported in tun"))
                   || lower.contains(QStringLiteral("may not be supported"))) {
            classified.append(ConfigWarning::warning(warning));
        } else {
            classified.append(ConfigWarning::info(warning));
        }
    }
    return classified;
}

QJsonObject SingBoxConfigGenerator::buildTunInbound() const
{
    QJsonObject tunInbound;
    tunInbound.insert(QStringLiteral("type"), QStringLiteral("tun"));
    tunInbound.insert(QStringLiteral("tag"), QStringLiteral("tun-in"));
    tunInbound.insert(QStringLiteral("interface_name"), QStringLiteral("zarya-tun"));
    tunInbound.insert(QStringLiteral("address"), QJsonArray{QStringLiteral("172.19.0.1/30")});
    tunInbound.insert(QStringLiteral("mtu"), 9000);
    tunInbound.insert(QStringLiteral("auto_route"), true);
    tunInbound.insert(QStringLiteral("strict_route"), true);
    tunInbound.insert(QStringLiteral("sniff"), true);
    return tunInbound;
}

QJsonObject SingBoxConfigGenerator::appendDnsHijackRoute(QJsonObject route,
                                                       QStringList* warnings) const
{
    QJsonArray rules = route.value(QStringLiteral("rules")).toArray();
    QJsonObject hijackRule;
    hijackRule.insert(QStringLiteral("protocol"), QStringLiteral("dns"));
    hijackRule.insert(QStringLiteral("action"), QStringLiteral("hijack-dns"));
    rules.prepend(hijackRule);
    route.insert(QStringLiteral("rules"), rules);
    if (warnings) {
        warnings->append(QStringLiteral(
            "DNS hijack is experimental and depends on your sing-box version."));
    }
    return route;
}

SingBoxConfigGenerationResult SingBoxConfigGenerator::generate(
    const Profile& profile, const RoutingProfile& routingProfile, const DnsProfile& dnsProfile,
    const SingBoxConfigOptions& options) const
{
    SingBoxConfigGenerationResult result;
    QStringList warnings;

    QString reason;
    if (!supportsProfile(profile, &reason)) {
        result.errorMessage = reason;
        result.classifiedWarnings.append(ConfigWarning::blocking(reason));
        return result;
    }

    QString outboundError;
    const QJsonObject proxyOutbound = buildProxyOutbound(profile, &outboundError);
    if (proxyOutbound.isEmpty()) {
        result.errorMessage = outboundError.isEmpty()
                                  ? QStringLiteral("Failed to build sing-box outbound.")
                                  : outboundError;
        result.classifiedWarnings.append(ConfigWarning::blocking(result.errorMessage));
        return result;
    }

    SingBoxRouteGenerationOptions routeOptions;
    routeOptions.tunMode = options.tunMode;
    routeOptions.enableAutoDetectInterface = options.enableAutoDetectInterface;
    routeOptions.enableRuleSets = options.enableRuleSets;
    routeOptions.finalOutbound = options.finalOutbound;

    const SingBoxRouteGenerator routeGenerator;
    QJsonObject route = routeGenerator.generateRoute(routingProfile, routeOptions, &warnings);

    if (options.enableDnsHijack) {
        route = appendDnsHijackRoute(route, &warnings);
    }

    SingBoxDnsGenerationOptions dnsOptions;
    dnsOptions.tunMode = options.tunMode;
    dnsOptions.enableDnsHijack = options.enableDnsHijack;

    const SingBoxDnsGenerator dnsGenerator;
    const QJsonObject dns = dnsGenerator.generateDns(dnsProfile, dnsOptions, &warnings);
    if (dns.isEmpty() && dnsProfile.mode != DnsProfileMode::System) {
        warnings.append(QStringLiteral("DNS section is empty; sing-box check is the final authority."));
    }

    QJsonObject directOutbound;
    directOutbound.insert(QStringLiteral("type"), QStringLiteral("direct"));
    directOutbound.insert(QStringLiteral("tag"), QStringLiteral("direct"));

    QJsonObject blockOutbound;
    blockOutbound.insert(QStringLiteral("type"), QStringLiteral("block"));
    blockOutbound.insert(QStringLiteral("tag"), QStringLiteral("block"));

    QJsonObject config;
    config.insert(QStringLiteral("log"), QJsonObject{{QStringLiteral("level"), QStringLiteral("info")}});
    if (!dns.isEmpty()) {
        config.insert(QStringLiteral("dns"), dns);
    }
    config.insert(QStringLiteral("inbounds"), QJsonArray{buildTunInbound()});
    config.insert(QStringLiteral("outbounds"),
                  QJsonArray{proxyOutbound, directOutbound, blockOutbound});
    config.insert(QStringLiteral("route"), route);

    result.success = true;
    result.config = config;
    mergeWarnings(result, warnings);
    result.classifiedWarnings = classifyWarnings(result.warnings);
    return result;
}

SingBoxConfigGenerationResult SingBoxConfigGenerator::generate(const Profile& profile) const
{
    return generate(profile, RoutingProfile::builtInProxyAll(), DnsProfile::builtInSystemDns(),
                  SingBoxConfigOptions{});
}

bool SingBoxConfigGenerator::supportsProfile(const Profile& profile, QString* reason) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        if (!profile.isSecurityReality()) {
            if (reason) {
                *reason = QStringLiteral("TUN mode supports VLESS with REALITY only.");
            }
            return false;
        }
        if (profile.network.trimmed().compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
            if (reason) {
                *reason = QStringLiteral("TUN mode supports VLESS REALITY over TCP only.");
            }
            return false;
        }
        if (profile.effectiveUuid().isEmpty() || profile.publicKey.trimmed().isEmpty()
            || profile.effectiveServerName().isEmpty()) {
            if (reason) {
                *reason = QStringLiteral("VLESS REALITY requires UUID, public key, and SNI.");
            }
            return false;
        }
        return true;
    case ProtocolType::Trojan:
        if (!profile.isSecurityTls()) {
            if (reason) {
                *reason = QStringLiteral("TUN mode supports Trojan with TLS only.");
            }
            return false;
        }
        if (profile.effectivePassword().isEmpty()) {
            if (reason) {
                *reason = QStringLiteral("Trojan requires password.");
            }
            return false;
        }
        return true;
    case ProtocolType::Shadowsocks:
        if (profile.hasUnsupportedFeature()) {
            if (reason) {
                *reason = profile.unsupportedReason;
            }
            return false;
        }
        return !profile.effectiveMethod().isEmpty() && !profile.effectivePassword().isEmpty();
    case ProtocolType::Vmess:
        if (profile.isSecurityReality()) {
            if (reason) {
                *reason = QStringLiteral("TUN mode does not support VMess REALITY.");
            }
            return false;
        }
        if (profile.effectiveUuid().isEmpty()) {
            if (reason) {
                *reason = QStringLiteral("VMess requires UUID.");
            }
            return false;
        }
        return true;
    default:
        if (reason) {
            *reason =
                QStringLiteral("Selected profile cannot be represented as sing-box outbound.");
        }
        return false;
    }
}

QJsonObject SingBoxConfigGenerator::buildProxyOutbound(const Profile& profile,
                                                       QString* errorMessage) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        return buildVlessOutbound(profile, errorMessage);
    case ProtocolType::Vmess:
        return buildVmessOutbound(profile, errorMessage);
    case ProtocolType::Trojan:
        return buildTrojanOutbound(profile, errorMessage);
    case ProtocolType::Shadowsocks:
        return buildShadowsocksOutbound(profile, errorMessage);
    default:
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported protocol.");
        }
        return {};
    }
}

QJsonObject SingBoxConfigGenerator::buildVlessOutbound(const Profile& profile,
                                                       QString* errorMessage) const
{
    QJsonObject outbound;
    outbound.insert(QStringLiteral("type"), QStringLiteral("vless"));
    outbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    outbound.insert(QStringLiteral("server"), profile.address.trimmed());
    outbound.insert(QStringLiteral("server_port"), profile.port);
    outbound.insert(QStringLiteral("uuid"), profile.effectiveUuid());
    const QString flow = profile.flow.trimmed();
    if (!flow.isEmpty()) {
        outbound.insert(QStringLiteral("flow"), flow);
    }
    outbound.insert(QStringLiteral("packet_encoding"), QStringLiteral("xudp"));

    QJsonObject tls;
    tls.insert(QStringLiteral("enabled"), true);
    tls.insert(QStringLiteral("server_name"), profile.effectiveServerName());
    tls.insert(QStringLiteral("utls"),
               QJsonObject{{QStringLiteral("enabled"), true},
                           {QStringLiteral("fingerprint"), profile.effectiveFingerprint()}});
    tls.insert(QStringLiteral("reality"),
               QJsonObject{{QStringLiteral("enabled"), true},
                           {QStringLiteral("public_key"), profile.publicKey.trimmed()},
                           {QStringLiteral("short_id"), profile.shortId.trimmed()}});
    outbound.insert(QStringLiteral("tls"), tls);
    Q_UNUSED(errorMessage);
    return outbound;
}

QJsonObject SingBoxConfigGenerator::buildVmessOutbound(const Profile& profile,
                                                       QString* errorMessage) const
{
    QJsonObject outbound;
    outbound.insert(QStringLiteral("type"), QStringLiteral("vmess"));
    outbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    outbound.insert(QStringLiteral("server"), profile.address.trimmed());
    outbound.insert(QStringLiteral("server_port"), profile.port);
    outbound.insert(QStringLiteral("uuid"), profile.effectiveUuid());
    outbound.insert(QStringLiteral("security"), profile.effectiveVmessSecurity());
    outbound.insert(QStringLiteral("alter_id"), profile.alterId);

    if (profile.isSecurityTls()) {
        QJsonObject tls;
        tls.insert(QStringLiteral("enabled"), true);
        tls.insert(QStringLiteral("server_name"), profile.effectiveServerName());
        if (!profile.effectiveFingerprint().isEmpty()) {
            tls.insert(QStringLiteral("utls"),
                       QJsonObject{{QStringLiteral("enabled"), true},
                                   {QStringLiteral("fingerprint"), profile.effectiveFingerprint()}});
        }
        outbound.insert(QStringLiteral("tls"), tls);
    }
    Q_UNUSED(errorMessage);
    return outbound;
}

QJsonObject SingBoxConfigGenerator::buildTrojanOutbound(const Profile& profile,
                                                        QString* errorMessage) const
{
    QJsonObject outbound;
    outbound.insert(QStringLiteral("type"), QStringLiteral("trojan"));
    outbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    outbound.insert(QStringLiteral("server"), profile.address.trimmed());
    outbound.insert(QStringLiteral("server_port"), profile.port);
    outbound.insert(QStringLiteral("password"), profile.effectivePassword());

    QJsonObject tls;
    tls.insert(QStringLiteral("enabled"), true);
    tls.insert(QStringLiteral("server_name"), profile.effectiveServerName());
    outbound.insert(QStringLiteral("tls"), tls);
    Q_UNUSED(errorMessage);
    return outbound;
}

QJsonObject SingBoxConfigGenerator::buildShadowsocksOutbound(const Profile& profile,
                                                             QString* errorMessage) const
{
    QJsonObject outbound;
    outbound.insert(QStringLiteral("type"), QStringLiteral("shadowsocks"));
    outbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    outbound.insert(QStringLiteral("server"), profile.address.trimmed());
    outbound.insert(QStringLiteral("server_port"), profile.port);
    outbound.insert(QStringLiteral("method"), profile.effectiveMethod());
    outbound.insert(QStringLiteral("password"), profile.effectivePassword());
    Q_UNUSED(errorMessage);
    return outbound;
}

} // namespace zarya
