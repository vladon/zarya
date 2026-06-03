#include "domain/SubscriptionStatus.h"

namespace zarya {

QString subscriptionStatusToString(SubscriptionStatus status)
{
    switch (status) {
    case SubscriptionStatus::NeverUpdated:
        return QStringLiteral("never_updated");
    case SubscriptionStatus::Updating:
        return QStringLiteral("updating");
    case SubscriptionStatus::Success:
        return QStringLiteral("success");
    case SubscriptionStatus::Failed:
        return QStringLiteral("failed");
    case SubscriptionStatus::Disabled:
        return QStringLiteral("disabled");
    }
    return QStringLiteral("never_updated");
}

SubscriptionStatus subscriptionStatusFromString(const QString& value,
                                                SubscriptionStatus defaultValue)
{
    if (value == QStringLiteral("updating")) {
        return SubscriptionStatus::Updating;
    }
    if (value == QStringLiteral("success")) {
        return SubscriptionStatus::Success;
    }
    if (value == QStringLiteral("failed")) {
        return SubscriptionStatus::Failed;
    }
    if (value == QStringLiteral("disabled")) {
        return SubscriptionStatus::Disabled;
    }
    if (value == QStringLiteral("never_updated")) {
        return SubscriptionStatus::NeverUpdated;
    }
    return defaultValue;
}

} // namespace zarya
