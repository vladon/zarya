#pragma once

#include <QString>

namespace zarya {

class BuildInfo {
public:
    static QString appVersion();
    static QString buildCommit();
    static QString buildDateUtc();
    static QString buildChannel();
    static QString qtVersion();
    static QString compilerInfo();

    static QString cliVersionText();
    static QString helperCliVersionText();
    static QString aboutText();
};

} // namespace zarya
