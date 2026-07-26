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

    QJsonObject mixedInbound;
    mixedInbound.insert(QStringLiteral("listen"), QStringLiteral("127.0.0.1"));
    mixedInbound.insert(QStringLiteral("port"), ports.mixedPort);
    mixedInbound.insert(QStringLiteral("protocol"), QStringLiteral("mixed"));
    mixedInbound.insert(QStringLiteral("tag"), QStringLiteral("mixed-in"));
    mixedInbound.insert(QStringLiteral("settings"), QJsonObject{
        {QStringLiteral("udp"), true},
    });

    QJsonObject defaultRule;
    defaultRule.insert(QStringLiteral("type"), QStringLiteral("field"));
    defaultRule.insert(QStringLiteral("network"), QStringLiteral("tcp,udp"));
    defaultRule.insert(QStringLiteral("outboundTag"), QStringLiteral("proxy"));

    QJsonObject config;
    config.insert(QStringLiteral("log"), QJsonObject{
        {QStringLiteral("loglevel"), QStringLiteral("warning")},
    });
    config.insert(QStringLiteral("inbounds"), QJsonArray{mixedInbound});
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
    return buildFullConfig(proxyOutbound, ports, routing, {});
}

QJsonObject XrayConfigBuilder::buildFullConfig(const QJsonObject& proxyOutbound,
                                                 const XrayInboundPorts& ports,
                                                 const QJsonObject& routing,
                                                 const QJsonObject& dns)
{
    QJsonObject config = buildFullConfig(proxyOutbound, ports);
    if (!routing.isEmpty()) {
        config.insert(QStringLiteral("routing"), routing);
    }
    if (!dns.isEmpty()) {
        config.insert(QStringLiteral("dns"), dns);
    }
    return config;
}

} // namespace zarya
