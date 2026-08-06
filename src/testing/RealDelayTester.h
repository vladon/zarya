#pragma once

#include "domain/Profile.h"
#include "testing/TestResult.h"

#include <atomic>
#include <functional>

namespace zarya {

class RealDelayTester {
public:
    using LogCallback = std::function<void(const QString&)>;

    static TestResult run(const Profile& profile, int timeoutMs, const QString& testUrl,
                          const LogCallback& log = {},
                          const std::atomic<bool>* cancelFlag = nullptr);
};

} // namespace zarya
