#pragma once

#include <QString>

namespace zarya {

struct SystemProxyState {
    bool proxyEnabled = false;
    QString proxyServer;
    QString proxyOverride;
    bool autoDetect = false;
    QString autoConfigUrl;
};

} // namespace zarya
