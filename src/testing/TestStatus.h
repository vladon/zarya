#pragma once

#include <QString>

namespace zarya {

enum class TestStatus {
    NeverTested,
    Testing,
    Available,
    Timeout,
    Failed,
    Unsupported,
    Canceled,
};

QString testStatusToString(TestStatus status);
TestStatus testStatusFromString(const QString& value);
QString testStatusDisplayString(TestStatus status);

} // namespace zarya
