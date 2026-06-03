#pragma once

#include "testing/TestStatus.h"

#include <QString>

namespace zarya {

struct TestResult {
    TestStatus status = TestStatus::NeverTested;
    int tcpPingMs = -1;
    int realDelayMs = -1;
    QString errorMessage;
};

} // namespace zarya
