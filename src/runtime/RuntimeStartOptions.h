#pragma once

#include "domain/DnsProfile.h"
#include "domain/RoutingProfile.h"

namespace zarya {

struct RuntimeStartOptions {
    bool fromAutostart = false;
    bool allowMissingPrivileges = false;
    bool useActiveRoutingProfile = true;
    bool useActiveDnsProfile = true;
    RoutingProfile routingProfile;
    DnsProfile dnsProfile;
    bool configWarningsAcknowledged = false;
};

} // namespace zarya
