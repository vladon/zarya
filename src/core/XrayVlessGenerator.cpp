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

ConfigGenerationResult XrayVlessGenerator::generate(const Profile& profile)
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

    return {true, buildFullConfig(proxyOutbound), {}};
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

QJsonObject XrayVlessGenerator::buildFullConfig(const QJsonObject& proxyOutbound)
{
    QJsonObject directOutbound;
    directOutbound.insert(QStringLiteral("tag"), QStringLiteral("direct"));
    directOutbound.insert(QStringLiteral("protocol"), QStringLiteral("freedom"));

    QJsonObject socksInbound;
    socksInbound.insert(QStringLiteral("listen"), QStringLiteral("127.0.0.1"));
    socksInbound.insert(QStringLiteral("port"), 10808);
    socksInbound.insert(QStringLiteral("protocol"), QStringLiteral("socks"));
    socksInbound.insert(QStringLiteral("tag"), QStringLiteral("socks-in"));
    socksInbound.insert(QStringLiteral("settings"), QJsonObject{
        {QStringLiteral("udp"), true},
    });

    QJsonObject httpInbound;
    httpInbound.insert(QStringLiteral("listen"), QStringLiteral("127.0.0.1"));
    httpInbound.insert(QStringLiteral("port"), 10809);
    httpInbound.insert(QStringLiteral("protocol"), QStringLiteral("http"));
    httpInbound.insert(QStringLiteral("tag"), QStringLiteral("http-in"));

    QJsonObject config;
    config.insert(QStringLiteral("log"), QJsonObject{
        {QStringLiteral("loglevel"), QStringLiteral("warning")},
    });
    config.insert(QStringLiteral("inbounds"), QJsonArray{socksInbound, httpInbound});
    config.insert(QStringLiteral("outbounds"), QJsonArray{proxyOutbound, directOutbound});
    config.insert(QStringLiteral("routing"), QJsonObject{
        {QStringLiteral("domainStrategy"), QStringLiteral("AsIs")},
        {QStringLiteral("rules"), QJsonArray{}},
    });
    return config;
}

QJsonObject XrayVlessGenerator::buildVlessUser(const Profile& profile)
{
    QJsonObject user;
    user.insert(QStringLiteral("id"), profile.uuidPassword);
    user.insert(QStringLiteral("encryption"), QStringLiteral("none"));

    QString flow = profile.flow.trimmed();
    if (profile.isSecurityReality() && flow.isEmpty()) {
        flow = QStringLiteral("xtls-rprx-vision");
    }
    if (!flow.isEmpty()) {
        user.insert(QStringLiteral("flow"), flow);
    }
    return user;
}

QJsonObject XrayVlessGenerator::buildStreamSettings(const Profile& profile,
                                                      QString* errorMessage)
{
    QJsonObject streamSettings;
    streamSettings.insert(QStringLiteral("network"), normalizedNetwork(profile));

    if (profile.isSecurityReality()) {
        if (normalizedNetwork(profile).compare(QStringLiteral("tcp"), Qt::CaseInsensitive) != 0) {
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
        streamSettings.insert(QStringLiteral("tlsSettings"), buildTlsSettings(profile));
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

    QString fingerprint = profile.fingerprint.trimmed();
    if (fingerprint.isEmpty()) {
        fingerprint = QStringLiteral("chrome");
    }
    reality.insert(QStringLiteral("fingerprint"), fingerprint);

    const QString serverName = profile.effectiveServerName();
    reality.insert(QStringLiteral("serverName"), serverName);
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
