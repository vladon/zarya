#pragma once

#include <QString>

namespace zarya {

class AppUpdatePaths {
public:
    static QString appUpdatesRootDir();
    static QString downloadsDir();
    static void ensureDirectories();
};

} // namespace zarya
