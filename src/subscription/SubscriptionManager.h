#pragma once

#include "domain/Subscription.h"

#include <QObject>
#include <QString>
#include <QVector>

namespace zarya {

struct Profile;
struct SubscriptionUpdateStats {
    int parsedProfiles = 0;
    int addedProfiles = 0;
    int updatedProfiles = 0;
    int markedMissingProfiles = 0;
    int skippedLines = 0;
};

struct SubscriptionUpdateResult {
    bool success = false;
    QString errorMessage;
    SubscriptionUpdateStats stats;
};

class SubscriptionManager : public QObject {
    Q_OBJECT

public:
    explicit SubscriptionManager(QObject* parent = nullptr);

    SubscriptionUpdateResult updateSubscription(Subscription& subscription,
                                                QVector<Profile>& profiles);
    QVector<SubscriptionUpdateResult> updateAll(QVector<Subscription>& subscriptions,
                                                QVector<Profile>& profiles);

signals:
    void logLine(const QString& line) const;

private:
    int countActiveProfilesForSubscription(const QVector<Profile>& profiles,
                                           const QString& subscriptionId) const;
};

} // namespace zarya
