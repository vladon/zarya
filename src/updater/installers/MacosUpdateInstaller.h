#pragma once

#include <QString>

namespace zarya {

class MacosUpdateInstaller {
public:
    static bool isImplemented() { return false; }

    static QString statusMessage()
    {
        return QStringLiteral(
            "Installed-mode macOS updates require signed DMG/PKG replacement. Not implemented yet.");
    }
};

} // namespace zarya
