#pragma once

#include "service/HelperServiceInstallOptions.h"

#include <QString>

namespace zarya {

class HelperServiceCommandLine {
public:
    static QString buildBinaryPath(const HelperServiceInstallOptions& options);
};

} // namespace zarya
