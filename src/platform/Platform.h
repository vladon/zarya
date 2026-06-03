#pragma once

#include <QString>

namespace zarya {

class Platform {
public:
    static QString defaultXrayExecutablePath();
    static QString defaultSingBoxExecutablePath();
};

} // namespace zarya
