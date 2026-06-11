#include "updater/AppUpdateStatus.h"

#include "packaging/InstallationMode.h"

namespace zarya {

AppUpdateStatus& AppUpdateStatus::instance()
{
    static AppUpdateStatus status;
    return status;
}

void AppUpdateStatus::recordCheckStarted()
{
    m_lastCheckResult = QStringLiteral("checking");
    m_lastCheckAt = QDateTime::currentDateTimeUtc();
}

void AppUpdateStatus::recordCheckSuccess(const QString& latestVersion)
{
    m_lastCheckResult = QStringLiteral("success");
    m_lastAvailableVersion = latestVersion;
    m_lastCheckAt = QDateTime::currentDateTimeUtc();
}

void AppUpdateStatus::recordCheckFailure(const QString& result)
{
    m_lastCheckResult = result;
    m_lastCheckAt = QDateTime::currentDateTimeUtc();
}

void AppUpdateStatus::reset()
{
    m_lastCheckResult = QStringLiteral("not_checked");
    m_lastAvailableVersion.clear();
    m_lastCheckAt = QDateTime();
}

AppUpdateChannel AppUpdateStatus::channel() const
{
    return m_channel;
}

void AppUpdateStatus::setChannel(AppUpdateChannel channel)
{
    m_channel = channel;
}

QString AppUpdateStatus::lastCheckResult() const
{
    return m_lastCheckResult;
}

QString AppUpdateStatus::lastAvailableVersion() const
{
    return m_lastAvailableVersion;
}

QDateTime AppUpdateStatus::lastCheckAt() const
{
    return m_lastCheckAt;
}

bool AppUpdateStatus::manifestUrlConfigured() const
{
    return m_manifestUrlConfigured;
}

void AppUpdateStatus::setManifestUrlConfigured(bool configured)
{
    m_manifestUrlConfigured = configured;
}

QJsonObject AppUpdateStatus::diagnosticsJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("channel"), AppUpdateChannelPolicy::toString(m_channel));
    object.insert(QStringLiteral("manifestUrlConfigured"), m_manifestUrlConfigured);
    if (m_lastCheckAt.isValid()) {
        object.insert(QStringLiteral("lastCheckAt"), m_lastCheckAt.toString(Qt::ISODate));
    } else {
        object.insert(QStringLiteral("lastCheckAt"), QJsonValue::Null);
    }
    object.insert(QStringLiteral("lastCheckResult"), m_lastCheckResult);
    if (m_lastAvailableVersion.isEmpty()) {
        object.insert(QStringLiteral("lastAvailableVersion"), QJsonValue::Null);
    } else {
        object.insert(QStringLiteral("lastAvailableVersion"), m_lastAvailableVersion);
    }
    object.insert(QStringLiteral("installationMode"), InstallationInfo::currentModeString());
    return object;
}

} // namespace zarya
