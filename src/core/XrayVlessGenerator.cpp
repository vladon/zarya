#include "core/XrayVlessGenerator.h"

#include "domain/ProfileValidation.h"

#include <QJsonArray>

namespace zarya {

namespace {

QString normalizedNetwork(const Profile& profile)
{
    return profile.network.trimmed().isEmpty() ? QStringLiteral("tcp")
                                               : profile.network.trimmed();
}

} // namespace

ConfigGenerationResult XrayVlessGenerator::generate(const Profile& profile,
                                                     const XrayInboundPorts& ports)
{
    const ProfileValidationResult validation = validateProfileForXray(profile);
    if (!validation.ok) {
        return {false, {}, validation.message};
    }

    QString error;
    const QJsonObject proxyOutbound = buildProxyOutbound(profile, &error);
    if (!error.isEmpty()) {
        return {false, {}, error};
    }

    return {true, buildFullConfig(proxyOutbound, ports), {}};
}

QJsonObject XrayVlessGenerator::buildProxyOutbound(const Profile& profile, QString* errorMessage)
{
    if (profile.protocol != ProtocolType::Vless) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Only VLESS is supported for Xray outbound generation.");
        }
        return {};
    }

    QJsonObject user = buildVlessUser(profile);

    QJsonObject vnextEntry;
    vnextEntry.insert(QStringLiteral("address"), profile.address);
    vnextEntry.insert(QStringLiteral("port"), profile.port);
    vnextEntry.insert(QStringLiteral("users"), QJsonArray{user});

    QJsonObject outboundSettings;
    outboundSettings.insert(QStringLiteral("vnext"), QJsonArray{vnextEntry});

    QString streamError;
    const QJsonObject streamSettings = buildStreamSettings(profile, &streamError);
    if (!streamError.isEmpty()) {
        if (errorMessage) {
            *errorMessage = streamError;
        }
        return {};
    }

    QJsonObject proxyOutbound;
    proxyOutbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    proxyOutbound.insert(QStringLiteral("protocol"), QStringLiteral("vless"));
    proxyOutbound.insert(QStringLiteral("settings"), outboundSettings);
    if (!streamSettings.isEmpty()) {
        proxyOutbound.insert(QStringLiteral("streamSettings"), streamSettings);
    }
    return proxyOutbound;
}

QJsonObject XrayVlessGenerator::buildFullConfig(const QJsonObject& proxyOutbound,
                                                const XrayInboundPorts& ports)
{
    QJsonObject directOutbound;
    directOutbound.insert(QStringLiteral("tag"), QStringLiteral("direct"));
    directOutbound.insert(QStringLiteral("protocol"), QStringLiteral("freedom"));

    QJsonObject blockOutbound;
    blockOutbound.insert(QStringLiteral("tag"), QStringLiteral("block"));
    blockOutbound.insert(QStringLiteral("protocol"), QStringLiteral("blackhole"));

    QJsonObject socksInbound;
    socksInbound.insert(QStringLiteral("listen"), QStringLiteral("127.0.0.1"));
    socksInbound.insert(QStringLiteral("port"), ports.socksPort);
    socksInbound.insert(QStringLiteral("protocol"), QStringLiteral("socks"));
    socksInbound.insert(QStringLiteral("tag"), QStringLiteral("socks-in"));
    socksInbound.insert(QStringLiteral("settings"), QJsonObject{
        {QStringLiteral("udp"), true},
    });

    QJsonObject httpInbound;
    httpInbound.insert(QStringLiteral("listen"), QStringLiteral("127.0.0.1"));
    httpInbound.insert(QStringLiteral("port"), ports.httpPort);
    httpInbound.insert(QStringLiteral("protocol"), QStringLiteral("http"));
    httpInbound.insert(QStringLiteral("tag"), QStringLiteral("http-in"));

    QJsonObject defaultRule;
    defaultRule.insert(QStringLiteral("type"), QStringLiteral("field"));
    defaultRule.insert(QStringLiteral("network"), QStringLiteral("tcp,udp"));
    defaultRule.insert(QStringLiteral("outboundTag"), QStringLiteral("proxy"));

    QJsonObject config;
    config.insert(QStringLiteral("log"), QJsonObject{
        {QStringLiteral("loglevel"), QStringLiteral("warning")},
    });
    config.insert(QStringLiteral("inbounds"), QJsonArray{socksInbound, httpInbound});
    config.insert(QStringLiteral("outbounds"),
                  QJsonArray{proxyOutbound, directOutbound, blockOutbound});
    config.insert(QStringLiteral("routing"), QJsonObject{
        {QStringLiteral("domainStrategy"), QStringLiteral("IPIfNonMatch")},
        {QStringLiteral("rules"), QJsonArray{defaultRule}},
    });
    return config;
}

QJsonObject XrayVlessGenerator::buildVlessUser(const Profile& profile)
{
    QJsonObject user;
    user.insert(QStringLiteral("id"), profile.uuidPassword);
    user.insert(QStringLiteral("encryption"), profile.effectiveEncryption());

    QString flow = profile.flow.trimmed();
    if (!flow.isEmpty()) {
        user.insert(QStringLiteral("flow"), flow);
    }
    return user;
}

QJsonObject XrayVlessGenerator::buildStreamSettings(const Profile& profile,
                                                      QString* errorMessage)
{
    QJsonObject streamSettings;
    const QString network = normalizedNetwork(profile);
    streamSettings.insert(QStringLiteral("network"), network);

    if (profile.isSecurityReality()) {
        if (network.compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("REALITY outbound requires network tcp.");
            }
            return {};
        }
        streamSettings.insert(QStringLiteral("security"), QStringLiteral("reality"));
        streamSettings.insert(QStringLiteral("realitySettings"), buildRealitySettings(profile));
        return streamSettings;
    }

    if (profile.isSecurityTls()) {
        streamSettings.insert(QStringLiteral("security"), QStringLiteral("tls"));
        QJsonObject tlsSettings = buildTlsSettings(profile);
        if (profile.allowInsecure) {
            tlsSettings.insert(QStringLiteral("allowInsecure"), true);
        }
        streamSettings.insert(QStringLiteral("tlsSettings"), tlsSettings);
        return streamSettings;
    }

    if (!profile.security.trimmed().isEmpty()
        && profile.security.compare(QStringLiteral("none"), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("Unsupported security type: %1").arg(profile.security);
        }
        return {};
    }

    return streamSettings;
}

QJsonObject XrayVlessGenerator::buildRealitySettings(const Profile& profile)
{
    QJsonObject reality;
    reality.insert(QStringLiteral("show"), false);
    reality.insert(QStringLiteral("fingerprint"), profile.effectiveFingerprint());
    reality.insert(QStringLiteral("serverName"), profile.effectiveServerName());
    reality.insert(QStringLiteral("publicKey"), profile.publicKey.trimmed());
    reality.insert(QStringLiteral("shortId"), profile.shortId.trimmed());

    QString spiderX = profile.spiderX.trimmed();
    if (spiderX.isEmpty()) {
        spiderX = QStringLiteral("/");
    }
    reality.insert(QStringLiteral("spiderX"), spiderX);

    return reality;
}

QJsonObject XrayVlessGenerator::buildTlsSettings(const Profile& profile)
{
    QJsonObject tlsSettings;
    const QString serverName = profile.effectiveServerName();
    if (!serverName.isEmpty()) {
        tlsSettings.insert(QStringLiteral("serverName"), serverName);
    } else {
        tlsSettings.insert(QStringLiteral("serverName"), profile.address);
    }
    return tlsSettings;
}

} // namespace zarya
