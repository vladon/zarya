#pragma once

#include <QString>

namespace zarya {

class LinuxUpdateInstaller {
public:
    static bool isImplemented() { return false; }

    static QString statusMessage()
    {
        return QStringLiteral(
            "Installed-mode Linux updates require package manager or AppImage replacement. Not "
            "implemented yet.");
    }
};

} // namespace zarya
