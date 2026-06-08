#pragma once

#include <QString>

namespace zarya {

struct CorePlatformInfo {
    QString osToken;
    QString archToken;
    QString displayName;
};

CorePlatformInfo currentCorePlatform();

} // namespace zarya
