#include "updater/AppUpdateChecker.h"

#include "app/BuildInfo.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"
#include "updater/AppUpdateChannel.h"
#include "updater/AppUpdateManifest.h"
#include "updater/AppUpdateStatus.h"

#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace zarya {

namespace {

constexpr int kManifestTimeoutMs = 20000;

QString redactManifestUrl(const QString& url)
{
    const QUrl parsed(url);
    if (!parsed.isValid()) {
        return url;
    }
    if (parsed.userName().isEmpty() && parsed.password().isEmpty() && parsed.query().isEmpty()) {
        return parsed.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery);
    }
    QUrl redacted = parsed;
    redacted.setUserName({});
    redacted.setPassword({});
    redacted.setQuery(QString());
    return redacted.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery);
}

} // namespace

AppUpdateChecker::AppUpdateChecker(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<AppUpdatePlan>();
}

void AppUpdateChecker::setLocalManifestPath(const QString& path)
{
    m_localManifestPath = path;
}

QString AppUpdateChecker::localManifestPath() const
{
    return m_localManifestPath;
}

void AppUpdateChecker::checkForUpdates()
{
    emit updateCheckStarted();
    AppUpdateStatus::instance().recordCheckStarted();

    const AppSettings& settings = AppSettings::instance();
    const AppUpdateChannel channel =
        AppUpdateChannelPolicy::fromString(settings.appUpdateChannelKey());
    AppUpdateStatus::instance().setChannel(channel);

    QString manifestSource;
    AppUpdateManifest manifest;
    QString errorMessage;

    if (!m_localManifestPath.isEmpty()) {
        manifestSource = m_localManifestPath;
        AppUpdateStatus::instance().setManifestUrlConfigured(true);
        if (!loadManifestFromFile(m_localManifestPath, &manifest, &errorMessage)) {
            AppUpdateStatus::instance().recordCheckFailure(QStringLiteral("failed"));
            emit updateCheckFailed(errorMessage);
            return;
        }
    } else {
        const QString manifestUrl = settings.appUpdateManifestUrl().trimmed();
        AppUpdateStatus::instance().setManifestUrlConfigured(!manifestUrl.isEmpty());
        if (manifestUrl.isEmpty()) {
            const QString message =
                QStringLiteral("Update manifest URL is not configured. Use Settings → Updates or "
                               "choose a local manifest file.");
            AppUpdateStatus::instance().recordCheckFailure(QStringLiteral("not_configured"));
            emit updateCheckFailed(message);
            return;
        }
        manifestSource = redactManifestUrl(manifestUrl);
        const QUrl url(manifestUrl);
        if (url.isLocalFile()) {
            if (!loadManifestFromFile(url.toLocalFile(), &manifest, &errorMessage)) {
                AppUpdateStatus::instance().recordCheckFailure(QStringLiteral("failed"));
                emit updateCheckFailed(errorMessage);
                return;
            }
        } else if (!loadManifestFromUrl(url, &manifest, &errorMessage)) {
            AppUpdateStatus::instance().recordCheckFailure(QStringLiteral("failed"));
            emit updateCheckFailed(errorMessage);
            return;
        }
    }

    Q_UNUSED(manifestSource);

    const AppUpdatePlan plan =
        AppUpdatePlanner::buildPlan(manifest, BuildInfo::appVersion(), channel);
    if (plan.updateAvailable) {
        AppUpdateStatus::instance().recordCheckSuccess(plan.latestVersion);
    } else {
        AppUpdateStatus::instance().recordCheckSuccess(plan.currentVersion);
    }
    emit updateCheckFinished(plan);
}

bool AppUpdateChecker::loadManifestFromFile(const QString& path, AppUpdateManifest* manifest,
                                            QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not read manifest: %1").arg(file.errorString());
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid update manifest JSON: %1")
                                .arg(parseError.errorString());
        }
        return false;
    }

    *manifest = AppUpdateManifest::fromJson(document.object(), errorMessage);
    return manifest->isValid(errorMessage);
}

bool AppUpdateChecker::loadManifestFromUrl(const QUrl& url, AppUpdateManifest* manifest,
                                           QString* errorMessage)
{
    if (!url.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid update manifest URL.");
        }
        return false;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(kManifestTimeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update manifest download timed out.");
        }
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = reply->errorString();
        reply->deleteLater();
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to download update manifest: %1").arg(message);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid update manifest JSON: %1")
                                .arg(parseError.errorString());
        }
        return false;
    }

    *manifest = AppUpdateManifest::fromJson(document.object(), errorMessage);
    return manifest->isValid(errorMessage);
}

} // namespace zarya
