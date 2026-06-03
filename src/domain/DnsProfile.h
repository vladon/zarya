#pragma once

#include "domain/DnsProfileMode.h"
#include "domain/DnsQueryStrategy.h"
#include "domain/DnsServer.h"

#include <QDateTime>
#include <QMap>
#include <QVector>

namespace zarya {

struct DnsProfile {
    QString id;
    QString name;
    DnsProfileMode mode = DnsProfileMode::System;
    bool enabled = true;
    bool isBuiltIn = false;
    DnsQueryStrategy queryStrategy = DnsQueryStrategy::UseSystemDefault;
    bool disableCache = false;
    bool disableFallback = false;
    bool disableFallbackIfMatch = false;
    QMap<QString, QString> hosts;
    QVector<DnsServer> servers;
    QDateTime createdAt;
    QDateTime updatedAt;

    static QVector<DnsProfile> createBuiltInProfiles();
    static DnsProfile builtInSystemDns();
};

} // namespace zarya
