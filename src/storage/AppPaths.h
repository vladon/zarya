#pragma once

#include <QString>

namespace zarya {

class AppPaths {
public:
    static QString appDataDir();
    static QString profilesFilePath();
    static QString subscriptionsFilePath();
    static QString routingFilePath();
    static QString runtimeDir();
    static QString xrayConfigPath();
    static QString singBoxConfigPath();
    static QString testRuntimeDir();
    static QString testConfigPath(const QString& profileId);
};

} // namespace zarya
