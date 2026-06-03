#include "core/XrayAdapter.h"

#include <QJsonArray>

namespace zarya {

CoreType XrayAdapter::type() const
{
    return CoreType::Xray;
}

QString XrayAdapter::displayName() const
{
    return QStringLiteral("Xray");
}

ConfigGenerationResult XrayAdapter::generateConfig(const Profile& profile) const
{
    switch (profile.protocol) {
    case ProtocolType::Vless:
        return generateVlessConfig(profile);
    case ProtocolType::Vmess:
    case ProtocolType::Trojan:
    case ProtocolType::Shadowsocks:
    case ProtocolType::Socks:
        return {false, {}, QStringLiteral("Protocol %1 is not implemented for Xray yet.")
                              .arg(protocolTypeToString(profile.protocol))};
    }
    return {false, {}, QStringLiteral("Unknown protocol.")};
}

QStringList XrayAdapter::argumentsForConfig(const QString& configPath) const
{
    return {QStringLiteral("run"), QStringLiteral("-c"), configPath};
}

ConfigGenerationResult XrayAdapter::generateVlessConfig(const Profile& profile) const
{
    QJsonObject user;
    user.insert(QStringLiteral("id"), profile.uuidPassword);
    user.insert(QStringLiteral("encryption"), QStringLiteral("none"));
    if (!profile.flow.isEmpty()) {
        user.insert(QStringLiteral("flow"), profile.flow);
    }

    QJsonObject vnextEntry;
    vnextEntry.insert(QStringLiteral("address"), profile.address);
    vnextEntry.insert(QStringLiteral("port"), profile.port);
    vnextEntry.insert(QStringLiteral("users"), QJsonArray{user});

    QJsonObject outboundSettings;
    outboundSettings.insert(QStringLiteral("vnext"), QJsonArray{vnextEntry});

    QJsonObject streamSettings;
    streamSettings.insert(QStringLiteral("network"), profile.network.isEmpty()
                                                        ? QStringLiteral("tcp")
                                                        : profile.network);
    if (!profile.security.isEmpty() && profile.security != QStringLiteral("none")) {
        QJsonObject tlsSettings;
        if (!profile.sni.isEmpty()) {
            tlsSettings.insert(QStringLiteral("serverName"), profile.sni);
        } else {
            tlsSettings.insert(QStringLiteral("serverName"), profile.address);
        }
        streamSettings.insert(QStringLiteral("security"), profile.security);
        streamSettings.insert(QStringLiteral("tlsSettings"), tlsSettings);
    }

    QJsonObject proxyOutbound;
    proxyOutbound.insert(QStringLiteral("tag"), QStringLiteral("proxy"));
    proxyOutbound.insert(QStringLiteral("protocol"), QStringLiteral("vless"));
    proxyOutbound.insert(QStringLiteral("settings"), outboundSettings);
    if (!streamSettings.isEmpty()) {
        proxyOutbound.insert(QStringLiteral("streamSettings"), streamSettings);
    }

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

    return {true, config, {}};
}

} // namespace zarya
