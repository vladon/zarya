#pragma once

#include <QString>

namespace zarya {

class AppPaths {
public:
    static QString appDataDir();
    static QString profilesFilePath();
    static QString subscriptionsFilePath();
    static QString runtimeDir();
    static QString xrayConfigPath();
    static QString singBoxConfigPath();
};

} // namespace zarya
