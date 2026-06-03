#include "core/SingBoxAdapter.h"

namespace zarya {

CoreType SingBoxAdapter::type() const
{
    return CoreType::SingBox;
}

QString SingBoxAdapter::displayName() const
{
    return QStringLiteral("sing-box");
}

ConfigGenerationResult SingBoxAdapter::generateConfig(const Profile& profile) const
{
    Q_UNUSED(profile);
    return {false,
            {},
            QStringLiteral("sing-box config generation is not implemented yet.")};
}

QStringList SingBoxAdapter::argumentsForConfig(const QString& configPath) const
{
    return {QStringLiteral("run"), QStringLiteral("-c"), configPath};
}

} // namespace zarya
