#pragma once

#include <QString>

namespace zarya {

class WindowsInstallInfo {
public:
    static bool isAvailable();
    static QString installerType();
    static QString installDirectory();
    static QString registryVersion();
    static bool helperServiceInstalled();
    static QString helperServiceState();
};

} // namespace zarya
