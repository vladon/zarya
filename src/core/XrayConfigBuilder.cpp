#include "core/XrayConfigBuilder.h"

#include <QJsonArray>

namespace zarya {

QJsonObject XrayConfigBuilder::buildFullConfig(const QJsonObject& proxyOutbound,
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
        {QStringLiteral("domainStrategy"), QStringLiteral("AsIs")},
        {QStringLiteral("rules"), QJsonArray{defaultRule}},
    });
    return config;
}

QJsonObject XrayConfigBuilder::buildFullConfig(const QJsonObject& proxyOutbound,
                                                 const XrayInboundPorts& ports,
                                                 const QJsonObject& routing)
{
    QJsonObject config = buildFullConfig(proxyOutbound, ports);
    if (!routing.isEmpty()) {
        config.insert(QStringLiteral("routing"), routing);
    }
    return config;
}

} // namespace zarya
