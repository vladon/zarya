#pragma once

#include <QString>

namespace zarya {

enum class SubscriptionStatus {
    NeverUpdated,
    Updating,
    Success,
    Failed,
    Disabled,
};

QString subscriptionStatusToString(SubscriptionStatus status);
SubscriptionStatus subscriptionStatusFromString(const QString& value,
                                                SubscriptionStatus defaultValue =
                                                    SubscriptionStatus::NeverUpdated);

} // namespace zarya
