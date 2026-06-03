#pragma once

#include <QString>

namespace zarya {

enum class CoreType {
    Xray,
    SingBox,
};

QString coreTypeToString(CoreType type);
CoreType coreTypeFromString(const QString& value, CoreType defaultValue = CoreType::Xray);

} // namespace zarya
