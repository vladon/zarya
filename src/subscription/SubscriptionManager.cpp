#include "subscription/SubscriptionManager.h"

#include "domain/ProfileSourceType.h"
#include "subscription/SubscriptionDownloader.h"
#include "subscription/SubscriptionParser.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QSet>

namespace zarya {

SubscriptionManager::SubscriptionManager(QObject* parent)
    : QObject(parent)
{
}

int SubscriptionManager::countActiveProfilesForSubscription(const QVector<Profile>& profiles,
                                                            const QString& subscriptionId) const
{
    int count = 0;
    for (const Profile& profile : profiles) {
        if (profile.isFromSubscription(subscriptionId) && !profile.deletedBySubscriptionUpdate) {
            ++count;
        }
    }
    return count;
}

SubscriptionUpdateResult SubscriptionManager::updateSubscription(Subscription& subscription,
                                                               QVector<Profile>& profiles)
{
    SubscriptionUpdateResult result;

    if (!subscription.enabled) {
        subscription.lastStatus = SubscriptionStatus::Disabled;
        subscription.lastError.clear();
        emit logLine(QStringLiteral("Subscription \"%1\" is disabled; skipped.")
                         .arg(subscription.name));
        result.success = true;
        return result;
    }

    emit logLine(QStringLiteral("Updating subscription: %1").arg(subscription.name));
    subscription.lastStatus = SubscriptionStatus::Updating;
    subscription.lastError.clear();

    const QString version =
        QCoreApplication::instance() ? QCoreApplication::applicationVersion()
                                     : QStringLiteral("0.4.0");
    const QString defaultUserAgent = QStringLiteral("Zarya/%1").arg(version);
    const QString userAgent =
        subscription.userAgent.trimmed().isEmpty() ? defaultUserAgent : subscription.userAgent.trimmed();

    emit logLine(QStringLiteral("Download started: %1").arg(subscription.url));
    SubscriptionDownloader downloader;
    const SubscriptionDownloadResult download = downloader.download(subscription.url, userAgent);
    if (!download.success) {
        subscription.lastStatus = SubscriptionStatus::Failed;
        subscription.lastError = download.errorMessage;
        result.errorMessage = download.errorMessage;
        emit logLine(QStringLiteral("Subscription update failed: %1").arg(download.errorMessage));
        return result;
    }

    emit logLine(QStringLiteral("HTTP status: %1").arg(download.httpStatusCode));
    emit logLine(QStringLiteral("Download size: %1 bytes").arg(download.body.size()));

    const SubscriptionParseResult parsed = SubscriptionParser::parse(download.body);
    if (!parsed.success) {
        subscription.lastStatus = SubscriptionStatus::Failed;
        subscription.lastError = parsed.errorMessage;
        result.errorMessage = parsed.errorMessage;
        emit logLine(QStringLiteral("Subscription update failed: %1").arg(parsed.errorMessage));
        return result;
    }

    result.stats.parsedProfiles = parsed.profiles.size();
    result.stats.skippedLines = parsed.skippedLines;
    emit logLine(QStringLiteral("Parsed profiles: %1").arg(parsed.profiles.size()));
    if (parsed.skippedLines > 0) {
        emit logLine(QStringLiteral("Skipped unsupported lines: %1").arg(parsed.skippedLines));
    }
    for (const QString& warning : parsed.warnings) {
        emit logLine(warning);
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    QHash<QString, int> existingIndexBySourceKey;
    for (int i = 0; i < profiles.size(); ++i) {
        const Profile& profile = profiles.at(i);
        if (profile.isFromSubscription(subscription.id) && !profile.sourceKey.isEmpty()) {
            existingIndexBySourceKey.insert(profile.sourceKey, i);
        }
    }

    QSet<QString> seenSourceKeys;
    for (const Profile& incomingTemplate : parsed.profiles) {
        Profile incoming = incomingTemplate;
        incoming.sourceType = ProfileSourceType::Subscription;
        incoming.subscriptionId = subscription.id;
        incoming.subscriptionName = subscription.name;
        incoming.lastSeenAt = now;
        incoming.deletedBySubscriptionUpdate = false;
        incoming.sourceKey = incoming.computeSourceKey();
        seenSourceKeys.insert(incoming.sourceKey);

        if (existingIndexBySourceKey.contains(incoming.sourceKey)) {
            Profile& existing = profiles[existingIndexBySourceKey.value(incoming.sourceKey)];
            incoming.id = existing.id;
            existing = incoming;
            ++result.stats.updatedProfiles;
        } else {
            profiles.append(incoming);
            existingIndexBySourceKey.insert(incoming.sourceKey, profiles.size() - 1);
            ++result.stats.addedProfiles;
        }
    }

    for (Profile& profile : profiles) {
        if (!profile.isFromSubscription(subscription.id)) {
            continue;
        }
        if (seenSourceKeys.contains(profile.sourceKey)) {
            continue;
        }
        if (!profile.deletedBySubscriptionUpdate) {
            profile.deletedBySubscriptionUpdate = true;
            ++result.stats.markedMissingProfiles;
        }
    }

    subscription.lastUpdatedAt = now;
    subscription.lastStatus = SubscriptionStatus::Success;
    subscription.lastError.clear();
    subscription.profileCount = countActiveProfilesForSubscription(profiles, subscription.id);

    emit logLine(QStringLiteral("Added profiles: %1").arg(result.stats.addedProfiles));
    emit logLine(QStringLiteral("Updated profiles: %1").arg(result.stats.updatedProfiles));
    emit logLine(QStringLiteral("Marked missing profiles: %1")
                     .arg(result.stats.markedMissingProfiles));
    emit logLine(QStringLiteral("Subscription update success."));

    result.success = true;
    return result;
}

QVector<SubscriptionUpdateResult> SubscriptionManager::updateAll(
    QVector<Subscription>& subscriptions, QVector<Profile>& profiles)
{
    QVector<SubscriptionUpdateResult> results;
    results.reserve(subscriptions.size());
    for (Subscription& subscription : subscriptions) {
        results.append(updateSubscription(subscription, profiles));
    }
    return results;
}

} // namespace zarya
