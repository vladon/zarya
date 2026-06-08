#pragma once

#include <QString>

namespace zarya {

struct DetectedVersion {
    bool ok = false;
    QString version;
    QString rawOutput;
    QString error;
};

} // namespace zarya
