#pragma once

#include "domain/Profile.h"

#include <QJsonObject>

namespace zarya {

struct SingBoxConfigGenerationResult {
    bool success = false;
    QJsonObject config;
    QString errorMessage;
};

class SingBoxConfigGenerator {
public:
    SingBoxConfigGenerationResult generate(const Profile& profile) const;
    bool supportsProfile(const Profile& profile, QString* reason = nullptr) const;

private:
    QJsonObject buildProxyOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildVlessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildVmessOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildTrojanOutbound(const Profile& profile, QString* errorMessage) const;
    QJsonObject buildShadowsocksOutbound(const Profile& profile, QString* errorMessage) const;
};

} // namespace zarya
