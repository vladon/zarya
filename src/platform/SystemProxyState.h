#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace zarya {

struct SystemProxyState {
    bool proxyEnabled = false;
    QString proxyServer;
    QString proxyOverride;
    bool autoDetect = false;
    QString autoConfigUrl;

    QString platform;
    QString backend;
    QVariantMap rawValues;
    QString activeNetworkService;
    QStringList affectedNetworkServices;
    QString supportLevel;
};

} // namespace zarya
