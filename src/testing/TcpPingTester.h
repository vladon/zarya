#pragma once

#include "domain/Profile.h"
#include "testing/TestResult.h"

namespace zarya {

class TcpPingTester {
public:
    static TestResult run(const Profile& profile, int timeoutMs);
};

} // namespace zarya
