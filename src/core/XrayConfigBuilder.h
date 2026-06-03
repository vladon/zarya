#pragma once

#include "core/XrayVlessGenerator.h"

#include <QJsonObject>

namespace zarya {

class XrayConfigBuilder {
public:
    static QJsonObject buildFullConfig(const QJsonObject& proxyOutbound, const XrayInboundPorts& ports);
};

} // namespace zarya
