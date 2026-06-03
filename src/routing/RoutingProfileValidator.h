#pragma once

#include "domain/RoutingProfile.h"

#include <QStringList>

namespace zarya {

class RoutingProfileValidator {
public:
    static QStringList warnings(const RoutingProfile& profile);
};

} // namespace zarya
