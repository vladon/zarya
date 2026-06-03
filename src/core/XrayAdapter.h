#pragma once

#include "core/ICoreAdapter.h"

namespace zarya {

class XrayAdapter : public ICoreAdapter {
public:
    CoreType type() const override;
    QString displayName() const override;
    ConfigGenerationResult generateConfig(const Profile& profile) const override;
    QStringList argumentsForConfig(const QString& configPath) const override;

};

} // namespace zarya
