#pragma once

#include <QString>

namespace zarya {

class PackagingInfo {
public:
    static QString versionString();
    static QString platformName();
    static QString artifactPlatformTag();
};

} // namespace zarya
