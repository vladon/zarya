#pragma once

#include <QString>

namespace zarya {

struct PrivilegeCheckResult {
    bool elevated = false;
    QString summary;
};

class PlatformPrivilege {
public:
    static PrivilegeCheckResult currentProcessPrivileges();
};

} // namespace zarya
