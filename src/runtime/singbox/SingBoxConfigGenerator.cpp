#include "runtime/singbox/SingBoxConfigGenerator.h"

#include <QJsonArray>

namespace zarya {

namespace {

QJsonObject buildBaseTunConfig(const QJsonObject& proxyOutbound)
{
    QJsonObject config;
    config.insert(QStringLiteral("log"), QJsonObject{{QStringLiteral("level"), QStringLiteral("info")}});

    QJsonObject remoteDns;
    remoteDns.insert(QStringLiteral("tag"), QStringLiteral("remote"));
    remoteDns.insert(QStringLiteral("address"), QStringLiteral("https://cloudflare-dns.com/dns-query"));
    remoteDns.insert(QStringLiteral("detour"), QStringLiteral("proxy"));

    QJsonObject localDns;
    localDns.insert(QStringLiteral("tag"), QStringLiteral("local"));
    localDns.insert(QStringLiteral("address"), QStringLiteral("local"));

    QJsonObject privateDnsRule;
    privateDnsRule.insert(QStringLiteral("geosite"), QStringLiteral("private"));
    privateDnsRule.insert(QStringLiteral("server"), QStringLiteral("local"));

    QJsonObject dns;
    dns.insert(QStringLiteral("servers"), QJsonArray{remoteDns, localDns});
    dns.insert(QStringLiteral("rules"), QJsonArray{privateDnsRule});
    dns.insert(QStringLiteral("final"), QStringLiteral("remote"));
    config.insert(QStringLiteral("dns"), dns);

    QJsonObject tunInbound;
    tunInbound.insert(QStringLiteral("type"), QStringLiteral("tun"));
    tunInbound.insert(QStringLiteral("tag"), QStringLiteral("tun-in"));
    tunInbound.insert(QStringLiteral("interface_name"), QStringLiteral("zarya-tun"));
    tunInbound.insert(QStringLiteral("address"), QJsonArray{QStringLiteral("172.19.0.1/30")});
    tunInbound.insert(QStringLiteral("mtu"), 9000);
    tunInbound.insert(QStringLiteral("auto_route"), true);
    tunInbound.insert(QStringLiteral("strict_route"), true);
    tunInbound.insert(QStringLiteral("sniff"), true);
    config.insert(QStringLiteral("inbounds"), QJsonArray{tunInbound});

    QJsonObject directOutbound;
    directOutbound.insert(QStringLiteral("type"), QStringLiteral("direct"));
    directOutbound.insert(QStringLiteral("tag"), QStringLiteral("direct"));

    QJsonObject blockOutbound;
    blockOutbound.insert(QStringLiteral("type"), QStringLiteral("block"));
    blockOutbound.insert(QStringLiteral("tag"), QStringLiteral("block"));

    config.insert(QStringLiteral("outbounds"),
                  QJsonArray{proxyOutbound, directOutbound, blockOutbound});

    QJsonObject route;
    route.insert(QStringLiteral("auto_detect_interface"), true);
    route.insert(QStringLiteral("final"), QStringLiteral("proxy"));
    config.insert(QStringLiteral("route"), route);

    return config;
}

} // namespace

bool SingBoxConfigGenerator::supportsProfile(const Profile& profile, QString* reason) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        if (!profile.isSecurityReality()) {
            if (reason) {
                *reason = QStringLiteral("TUN PoC supports VLESS with REALITY only.");
            }
            return false;
        }
        if (profile.network.trimmed().compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
            if (reason) {
                *reason = QStringLiteral("TUN PoC supports VLESS REALITY over TCP only.");
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
                *reason = QStringLiteral("TUN PoC supports Trojan with TLS only.");
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
                *reason = QStringLiteral("TUN PoC does not support VMess REALITY.");
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
            *reason = QStringLiteral("Protocol is not supported by experimental TUN mode.");
        }
        return false;
    }
}

SingBoxConfigGenerationResult SingBoxConfigGenerator::generate(const Profile& profile) const
{
    SingBoxConfigGenerationResult result;
    QString reason;
    if (!supportsProfile(profile, &reason)) {
        result.errorMessage = reason;
        return result;
    }

    QString outboundError;
    const QJsonObject proxyOutbound = buildProxyOutbound(profile, &outboundError);
    if (proxyOutbound.isEmpty()) {
        result.errorMessage = outboundError.isEmpty()
                                  ? QStringLiteral("Failed to build sing-box outbound.")
                                  : outboundError;
        return result;
    }

    result.success = true;
    result.config = buildBaseTunConfig(proxyOutbound);
    return result;
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
