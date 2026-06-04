#pragma once

#include <QString>

namespace zarya {

class Platform {
public:
    static QString defaultXrayExecutablePath();
    static QString defaultSingBoxExecutablePath();
    static QString defaultHelperExecutablePath();
};

} // namespace zarya
