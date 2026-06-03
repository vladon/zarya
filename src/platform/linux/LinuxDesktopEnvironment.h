#pragma once

#include <QString>

namespace zarya {

enum class LinuxDesktopEnvironment {
    Gnome,
    Kde,
    Unknown,
};

class LinuxDesktopEnvironmentDetector {
public:
    static LinuxDesktopEnvironment detect();
    static QString detectDisplayName();
};

} // namespace zarya
