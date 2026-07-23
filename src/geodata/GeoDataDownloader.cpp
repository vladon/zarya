#include "geodata/GeoDataDownloader.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace zarya {

namespace {

constexpr int kIdleTimeoutMs = 30000;
constexpr int kMinFileTimeoutMs = 300000; // 5 minutes floor for geo/rule-set archives

void configureDirectRequest(QNetworkAccessManager* manager, QNetworkRequest* request,
                            const QString& userAgent)
{
    manager->setProxy(QNetworkProxy::NoProxy);
    request->setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request->setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);
}

} // namespace

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
    configureDirectRequest(&manager, &request, userAgent);

    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer overallTimer;
    overallTimer.setSingleShot(true);
    const int effectiveTimeoutMs = qMax(m_timeoutMs, kMinFileTimeoutMs);

    QTimer idleTimer;
    idleTimer.setSingleShot(true);

    if (progressCallback) {
        QObject::connect(reply, &QNetworkReply::downloadProgress,
                         [&](qint64 received, qint64 total) {
                             idleTimer.start(kIdleTimeoutMs);
                             progressCallback(received, total);
                         });
    }

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&overallTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&idleTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

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

    overallTimer.start(effectiveTimeoutMs);
    idleTimer.start(kIdleTimeoutMs);
    loop.exec();

    if (cancelCallback && cancelCallback()) {
        result.errorMessage = QStringLiteral("Download canceled.");
        reply->deleteLater();
        return result;
    }

    // Prefer a completed reply over a timer race — progress can hit 100% just as the
    // overall timer expires, which previously reported a false timeout.
    if (!reply->isFinished()) {
        reply->abort();
        if (!overallTimer.isActive()) {
            result.errorMessage = QStringLiteral("Download timed out.");
        } else {
            result.errorMessage = QStringLiteral("Download stalled (no data received).");
        }
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
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };

    if (!url.isValid()) {
        return fail(QStringLiteral("Invalid download URL."));
    }

    QFile::remove(destinationPath);
    QFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail(file.errorString());
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    configureDirectRequest(&manager, &request, userAgent);

    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer overallTimer;
    overallTimer.setSingleShot(true);
    const int effectiveTimeoutMs = qMax(m_timeoutMs, kMinFileTimeoutMs);

    QTimer idleTimer;
    idleTimer.setSingleShot(true);

    bool writeFailed = false;
    QObject::connect(reply, &QNetworkReply::readyRead, &file, [&]() {
        const QByteArray chunk = reply->readAll();
        if (chunk.isEmpty()) {
            return;
        }
        if (file.write(chunk) != chunk.size()) {
            writeFailed = true;
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::downloadProgress,
                     [&](qint64 received, qint64 total) {
                         idleTimer.start(kIdleTimeoutMs);
                         if (progressCallback) {
                             progressCallback(received, total);
                         }
                     });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&overallTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&idleTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

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

    overallTimer.start(effectiveTimeoutMs);
    idleTimer.start(kIdleTimeoutMs);
    loop.exec();

    if (reply->bytesAvailable() > 0 && !writeFailed) {
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty() && file.write(chunk) != chunk.size()) {
            writeFailed = true;
        }
    }
    file.close();

    if (writeFailed) {
        QFile::remove(destinationPath);
        reply->deleteLater();
        return fail(QStringLiteral("Failed to write downloaded file."));
    }

    if (cancelCallback && cancelCallback()) {
        QFile::remove(destinationPath);
        reply->deleteLater();
        return fail(QStringLiteral("Download canceled."));
    }

    if (!reply->isFinished()) {
        reply->abort();
        QFile::remove(destinationPath);
        reply->deleteLater();
        if (!overallTimer.isActive()) {
            return fail(QStringLiteral("Download timed out."));
        }
        return fail(QStringLiteral("Download stalled (no data received)."));
    }

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        QFile::remove(destinationPath);
        reply->deleteLater();
        return fail(QStringLiteral("Download canceled."));
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString error = reply->errorString();
        QFile::remove(destinationPath);
        reply->deleteLater();
        return fail(error);
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if (status != 0 && (status < 200 || status >= 300)) {
        QFile::remove(destinationPath);
        return fail(QStringLiteral("Unexpected HTTP status: %1").arg(status));
    }

    if (!QFileInfo::exists(destinationPath) || QFileInfo(destinationPath).size() <= 0) {
        QFile::remove(destinationPath);
        return fail(QStringLiteral("Download returned empty body."));
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

} // namespace zarya
