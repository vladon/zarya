#pragma once

#include <QString>

namespace zarya {

// Design stub for future portable in-place update (zarya-updater external process).
class PortableUpdateInstaller {
public:
    static bool isImplemented() { return false; }

    static QString statusMessage()
    {
        return QStringLiteral("Portable app self-install is not enabled in this beta.");
    }
};

} // namespace zarya
