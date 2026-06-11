#pragma once

#include <QString>

namespace zarya {

struct HelperServiceInstallOptions {
    QString helperExecutablePath;
    QString serviceName;
    QString displayName;

    QString allowedRuntimeDir;
    QString allowedCoreDir;
    QString logFilePath;

    bool startAfterInstall = true;
    bool manualStart = true;
    bool installForAllUsers = false;

    QString allowedUserId;

    static HelperServiceInstallOptions defaultsForCurrentApp(const QString& helperExecutablePath);
};

} // namespace zarya
