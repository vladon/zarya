#include "cores/CoreDownloader.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace zarya {

namespace {

constexpr int kIdleTimeoutMs = 30000;
constexpr int kMinFileTimeoutMs = 300000; // 5 minutes floor for archive downloads

} // namespace

CoreDownloader::CoreDownloader(QObject* parent)
    : QObject(parent)
{
}

void CoreDownloader::cancel()
{
    m_cancelled = true;
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

bool CoreDownloader::downloadToFile(const QUrl& url, const QString& destinationPath,
                                    const QString& userAgent, int timeoutMs,
                                    QString* errorMessage)
{
    m_cancelled = false;
    m_activeReply.clear();

    auto fail = [this, errorMessage](const QString& message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        emit finished(false, message);
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
    // Avoid routing GitHub CDN traffic through an active system / Zarya proxy.
    manager.setProxy(QNetworkProxy::NoProxy);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager.get(request);
    m_activeReply = reply;

    QEventLoop loop;
    QTimer overallTimer;
    overallTimer.setSingleShot(true);
    const int effectiveTimeoutMs = qMax(timeoutMs, kMinFileTimeoutMs);

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
    QObject::connect(reply, &QNetworkReply::downloadProgress, this,
                     [this, &idleTimer](qint64 received, qint64 total) {
                         idleTimer.start(kIdleTimeoutMs);
                         emit progress(received, total);
                     });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&overallTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&idleTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    QTimer cancelPoll;
    cancelPoll.setInterval(100);
    QObject::connect(&cancelPoll, &QTimer::timeout, [&]() {
        if (m_cancelled && reply) {
            reply->abort();
        }
    });
    cancelPoll.start();

    overallTimer.start(effectiveTimeoutMs);
    idleTimer.start(kIdleTimeoutMs);
    loop.exec();

    cancelPoll.stop();
    m_activeReply.clear();

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

    if (m_cancelled || reply->error() == QNetworkReply::OperationCanceledError) {
        QFile::remove(destinationPath);
        reply->deleteLater();
        return fail(QStringLiteral("Download cancelled."));
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
    emit finished(true, {});
    return true;
}

} // namespace zarya
