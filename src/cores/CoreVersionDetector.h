#pragma once

#include "cores/DetectedVersion.h"
#include "domain/CoreType.h"

#include <QString>

namespace zarya {

class CoreVersionDetector {
public:
    static DetectedVersion detect(const QString& executablePath, CoreType type,
                                  int timeoutMs = 5000);
    static QString normalizeVersion(QString version);
};

} // namespace zarya
