#pragma once

#include "core/ICoreAdapter.h"
#include "core/XrayVlessGenerator.h"
#include "domain/DnsProfile.h"
#include "domain/RoutingProfile.h"

#include <QJsonObject>

namespace zarya {

class XrayAdapter : public ICoreAdapter {
public:
    CoreType type() const override;
    QString displayName() const override;
    ConfigGenerationResult generateConfig(const Profile& profile) const override;
    ConfigGenerationResult generateConfig(const Profile& profile,
                                          const XrayInboundPorts& ports) const;
    ConfigGenerationResult generateConfig(const Profile& profile, const XrayInboundPorts& ports,
                                          const RoutingProfile& routingProfile) const;
    ConfigGenerationResult generateConfig(const Profile& profile, const XrayInboundPorts& ports,
                                          const RoutingProfile& routingProfile,
                                          const DnsProfile& dnsProfile) const;
    QStringList argumentsForConfig(const QString& configPath) const override;

    bool supportsProfile(const Profile& profile, QString* reason = nullptr) const;
    QJsonObject generateOutbound(const Profile& profile, QString* errorMessage = nullptr) const;

private:
    ConfigGenerationResult generateConfigInternal(const Profile& profile,
                                                  const XrayInboundPorts& ports,
                                                  const RoutingProfile* routingProfile,
                                                  const DnsProfile* dnsProfile) const;

    QJsonObject generateVlessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject generateVmessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject generateTrojanOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject generateShadowsocksOutbound(const Profile& profile,
                                            QString* errorMessage) const;
    QJsonObject generateSocksOutbound(const Profile& profile, QString* errorMessage) const;

    static QJsonObject wrapProxyOutbound(const QString& protocol, const QJsonObject& settings,
                                         const QJsonObject& streamSettings);
};

} // namespace zarya
