#include "cores/GitHubReleaseProvider.h"

#include "packaging/PackagingInfo.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace zarya {

GitHubReleaseProvider::GitHubReleaseProvider(CoreType type, const QString& repoSlug,
                                             QObject* parent)
    : CoreReleaseProvider(parent)
    , m_type(type)
    , m_repoSlug(repoSlug)
{
}

CoreType GitHubReleaseProvider::coreType() const
{
    return m_type;
}

QString GitHubReleaseProvider::providerName() const
{
    return QStringLiteral("GitHub Releases (%1)").arg(m_repoSlug);
}

void GitHubReleaseProvider::fetchLatestRelease(int timeoutMs)
{
    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(m_repoSlug));
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString()));
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        emit error(QStringLiteral("GitHub API request timed out."));
        reply->deleteLater();
        return;
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        QString message = reply->errorString();
        if (status == 403) {
            message = QStringLiteral(
                "GitHub API rate limit reached. Try again later or install manually from %1")
                          .arg(url.toString());
        }
        emit error(message);
        reply->deleteLater();
        return;
    }

    const QJsonObject object = QJsonDocument::fromJson(body).object();
    CoreRelease release;
    release.coreType = m_type;
    release.version = object.value(QStringLiteral("tag_name")).toString();
    release.htmlUrl = QUrl(object.value(QStringLiteral("html_url")).toString());
    release.releaseNotes = object.value(QStringLiteral("body")).toString();
    release.publishedAt =
        QDateTime::fromString(object.value(QStringLiteral("published_at")).toString(), Qt::ISODate);

    const QJsonArray assets = object.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject assetObject = value.toObject();
        CoreAsset asset;
        asset.name = assetObject.value(QStringLiteral("name")).toString();
        asset.downloadUrl = QUrl(assetObject.value(QStringLiteral("browser_download_url")).toString());
        asset.sizeBytes = assetObject.value(QStringLiteral("size")).toDouble();
        release.assets.append(asset);
    }

    reply->deleteLater();
    emit latestReleaseReady(release);
}

} // namespace zarya
