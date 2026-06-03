#include "domain/Subscription.h"

#include <QUuid>

namespace zarya {

Subscription Subscription::createDefault()
{
    Subscription subscription;
    subscription.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    subscription.name = QStringLiteral("New subscription");
    subscription.enabled = true;
    subscription.lastStatus = SubscriptionStatus::NeverUpdated;
    return subscription;
}

} // namespace zarya
