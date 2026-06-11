#pragma once

#include "updater/AppUpdateChannel.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>

namespace zarya {

class AppUpdateStatus {
public:
    static AppUpdateStatus& instance();

    void recordCheckStarted();
    void recordCheckSuccess(const QString& latestVersion);
    void recordCheckFailure(const QString& result);
    void recordDownloadVerified(const QString& artifactFileName);
    void recordInstallAttempt(const QString& status);
    void reset();

    AppUpdateChannel channel() const;
    void setChannel(AppUpdateChannel channel);

    QString lastCheckResult() const;
    QString lastAvailableVersion() const;
    QDateTime lastCheckAt() const;
    bool manifestUrlConfigured() const;
    void setManifestUrlConfigured(bool configured);

    QJsonObject diagnosticsJson() const;

private:
    AppUpdateStatus() = default;

    AppUpdateChannel m_channel = AppUpdateChannel::Beta;
    QString m_lastCheckResult = QStringLiteral("not_checked");
    QString m_lastAvailableVersion;
    QDateTime m_lastCheckAt;
    bool m_manifestUrlConfigured = false;
};

} // namespace zarya
