#pragma once

#include "domain/RoutingMode.h"
#include "domain/RoutingRule.h"

#include <QDateTime>
#include <QVector>

namespace zarya {

struct RoutingProfile {
    QString id;
    QString name;
    RoutingMode mode = RoutingMode::ProxyAll;
    bool enabled = true;
    QString domainStrategy = QStringLiteral("AsIs");
    QVector<RoutingRule> rules;
    QDateTime createdAt;
    QDateTime updatedAt;
    bool isBuiltIn = false;

    static QVector<RoutingProfile> createBuiltInProfiles();
    static RoutingProfile builtInProxyAll();
    static bool isValidDomainStrategy(const QString& strategy);
};

} // namespace zarya
