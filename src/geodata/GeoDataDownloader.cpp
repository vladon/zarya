#include "geodata/GeoDataDownloader.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace zarya {

GeoDataDownloader::GeoDataDownloader(int timeoutMs)
    : m_timeoutMs(timeoutMs)
{
}

GeoDataDownloadResult GeoDataDownloader::download(
    const QUrl& url, const QString& userAgent,
    const std::function<void(qint64, qint64)>& progressCallback,
    const std::function<bool()>& cancelCallback) const
{
    GeoDataDownloadResult result;
    if (!url.isValid()) {
        result.errorMessage = QStringLiteral("Invalid download URL.");
        return result;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager.get(request);

    if (progressCallback) {
        QObject::connect(reply, &QNetworkReply::downloadProgress, [&](qint64 received, qint64 total) {
            progressCallback(received, total);
        });
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer cancelTimer;
    if (cancelCallback) {
        cancelTimer.setInterval(100);
        QObject::connect(&cancelTimer, &QTimer::timeout, [&]() {
            if (cancelCallback()) {
                reply->abort();
            }
        });
        cancelTimer.start();
    }

    timer.start(m_timeoutMs);
    loop.exec();

    if (cancelCallback && cancelCallback()) {
        result.errorMessage = QStringLiteral("Download canceled.");
        reply->deleteLater();
        return result;
    }

    if (!timer.isActive()) {
        reply->abort();
        result.errorMessage = QStringLiteral("Download timed out.");
        reply->deleteLater();
        return result;
    }

    result.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.body = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (result.httpStatusCode < 200 || result.httpStatusCode >= 300) {
        result.errorMessage =
            QStringLiteral("Unexpected HTTP status: %1").arg(result.httpStatusCode);
        return result;
    }

    if (result.body.isEmpty()) {
        result.errorMessage = QStringLiteral("Download returned empty body.");
        return result;
    }

    result.success = true;
    return result;
}

bool GeoDataDownloader::downloadToFile(const QUrl& url, const QString& destinationPath,
                                         const QString& userAgent, QString* errorMessage,
                                         const std::function<void(qint64, qint64)>& progressCallback,
                                         const std::function<bool()>& cancelCallback) const
{
    const GeoDataDownloadResult result = download(url, userAgent, progressCallback, cancelCallback);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = result.errorMessage;
        }
        return false;
    }

    QFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    if (file.write(result.body) != result.body.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write downloaded file.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
