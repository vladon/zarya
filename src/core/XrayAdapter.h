#pragma once

#include "core/ICoreAdapter.h"
#include "core/XrayVlessGenerator.h"

namespace zarya {

class XrayAdapter : public ICoreAdapter {
public:
    CoreType type() const override;
    QString displayName() const override;
    ConfigGenerationResult generateConfig(const Profile& profile) const override;
    ConfigGenerationResult generateConfig(const Profile& profile,
                                          const XrayInboundPorts& ports) const;
    QStringList argumentsForConfig(const QString& configPath) const override;

    bool supportsProfile(const Profile& profile, QString* reason = nullptr) const;

private:
    ConfigGenerationResult generateConfigInternal(const Profile& profile,
                                                  const XrayInboundPorts& ports) const;
};

} // namespace zarya
