#pragma once

#include <QString>

namespace zarya {

enum class ProfileSourceType {
    Manual,
    Subscription,
};

QString profileSourceTypeToString(ProfileSourceType type);
ProfileSourceType profileSourceTypeFromString(const QString& value,
                                              ProfileSourceType defaultValue =
                                                  ProfileSourceType::Manual);

} // namespace zarya
