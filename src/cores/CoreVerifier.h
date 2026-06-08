#pragma once

#include "cores/DetectedVersion.h"
#include "domain/CoreType.h"

#include <QString>

namespace zarya {

class CoreVerifier {
public:
    static QString executableFileName(CoreType type);
    static QString findExecutableInTree(const QString& rootDir, CoreType type);
    static bool verifyStaged(const QString& executablePath, CoreType type,
                             const QString& expectedVersion, QString* errorMessage);
};

} // namespace zarya
