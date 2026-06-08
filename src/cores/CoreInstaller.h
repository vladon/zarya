#pragma once

#include "domain/CoreType.h"

#include <QString>

namespace zarya {

class CoreInstaller {
public:
    static bool installFromStaging(CoreType type, const QString& stagedExecutablePath,
                                   const QString& installDir, const QString& version,
                                   QString* errorMessage);
};

} // namespace zarya
