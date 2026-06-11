#pragma once

#include <QString>

namespace zarya {

class WindowsInstalledUpdateInstaller {
public:
    static bool isImplemented() { return false; }

    static QString statusMessage()
    {
        return QStringLiteral(
            "Installed-mode Windows updates require an MSI/installer. Not implemented yet.");
    }
};

} // namespace zarya
