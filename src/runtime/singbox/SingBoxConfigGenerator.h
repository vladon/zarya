#pragma once

#include "domain/DnsProfile.h"
#include "domain/Profile.h"
#include "domain/RoutingProfile.h"
#include "runtime/ConfigWarning.h"

#include <QJsonObject>
#include <QStringList>

namespace zarya {

struct SingBoxConfigOptions {
    bool tunMode = true;
    bool enableDnsHijack = true;
    bool enableAutoDetectInterface = true;
    bool enableRuleSets = true;
    QString finalOutbound = QStringLiteral("proxy");
};

struct SingBoxConfigGenerationResult {
    bool success = false;
    QJsonObject config;
    QString errorMessage;
    QStringList warnings;
    QList<ConfigWarning> classifiedWarnings;
};

class SingBoxConfigGenerator {
public:
    SingBoxConfigGenerationResult generate(const Profile& profile,
                                           const RoutingProfile& routingProfile,
                                           const DnsProfile& dnsProfile,
                                           const SingBoxConfigOptions& options) const;

    SingBoxConfigGenerationResult generate(const Profile& profile) const;

    bool supportsProfile(const Profile& profile, QString* reason = nullptr) const;

private:
    QJsonObject buildTunInbound() const;
    QJsonObject appendDnsHijackRoute(QJsonObject route, QStringList* warnings) const;

    QJsonObject buildProxyOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildVlessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildVmessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildTrojanOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildShadowsocksOutbound(const Profile& profile, QString* errorMessage) const;

    QList<ConfigWarning> classifyWarnings(const QStringList& warnings) const;
};

} // namespace zarya
