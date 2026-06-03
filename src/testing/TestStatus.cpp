#include "testing/TestStatus.h"

namespace zarya {

QString testStatusToString(TestStatus status)
{
    switch (status) {
    case TestStatus::NeverTested:
        return QStringLiteral("never_tested");
    case TestStatus::Testing:
        return QStringLiteral("testing");
    case TestStatus::Available:
        return QStringLiteral("available");
    case TestStatus::Timeout:
        return QStringLiteral("timeout");
    case TestStatus::Failed:
        return QStringLiteral("failed");
    case TestStatus::Unsupported:
        return QStringLiteral("unsupported");
    case TestStatus::Canceled:
        return QStringLiteral("canceled");
    }
    return QStringLiteral("never_tested");
}

TestStatus testStatusFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("testing")) {
        return TestStatus::Testing;
    }
    if (normalized == QStringLiteral("available")) {
        return TestStatus::Available;
    }
    if (normalized == QStringLiteral("timeout")) {
        return TestStatus::Timeout;
    }
    if (normalized == QStringLiteral("failed")) {
        return TestStatus::Failed;
    }
    if (normalized == QStringLiteral("unsupported")) {
        return TestStatus::Unsupported;
    }
    if (normalized == QStringLiteral("canceled")) {
        return TestStatus::Canceled;
    }
    return TestStatus::NeverTested;
}

QString testStatusDisplayString(TestStatus status)
{
    switch (status) {
    case TestStatus::NeverTested:
        return QStringLiteral("Never tested");
    case TestStatus::Testing:
        return QStringLiteral("Testing");
    case TestStatus::Available:
        return QStringLiteral("Available");
    case TestStatus::Timeout:
        return QStringLiteral("Timeout");
    case TestStatus::Failed:
        return QStringLiteral("Failed");
    case TestStatus::Unsupported:
        return QStringLiteral("Unsupported");
    case TestStatus::Canceled:
        return QStringLiteral("Canceled");
    }
    return QStringLiteral("Never tested");
}

} // namespace zarya
