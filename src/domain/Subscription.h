#pragma once

#include "domain/SubscriptionStatus.h"

#include <QDateTime>
#include <QString>

namespace zarya {

struct Subscription {
    QString id;
    QString name;
    QString url;
    bool enabled = true;
    QDateTime lastUpdatedAt;
    SubscriptionStatus lastStatus = SubscriptionStatus::NeverUpdated;
    QString lastError;
    int profileCount = 0;
    QString userAgent;
    QString remarks;

    static Subscription createDefault();
};

} // namespace zarya
