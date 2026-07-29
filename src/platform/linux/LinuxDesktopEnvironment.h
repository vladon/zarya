#pragma once

#include <QProcessEnvironment>
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
    static LinuxDesktopEnvironment detect(const QProcessEnvironment& environment);
    static QString detectDisplayName();
    static QString displayName(LinuxDesktopEnvironment environment);
};

} // namespace zarya
