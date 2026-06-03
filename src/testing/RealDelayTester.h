#pragma once

#include "domain/Profile.h"
#include "testing/TestResult.h"

#include <functional>

namespace zarya {

class RealDelayTester {
public:
    using LogCallback = std::function<void(const QString&)>;

    static TestResult run(const Profile& profile, int timeoutMs, const QString& testUrl,
                          const LogCallback& log = {});
};

} // namespace zarya
