#pragma once

#include <QString>

namespace zarya {

struct HelperServiceIdentity {
    static QString internalServiceName();
    static QString displayName();
    static QString macOsLabel();
    static QString linuxUnitName();
    static QString polkitActionPrefix();
};

} // namespace zarya
