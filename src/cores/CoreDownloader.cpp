#include "cores/CoreDownloader.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace zarya {

CoreDownloader::CoreDownloader(QObject* parent)
    : QObject(parent)
{
}

void CoreDownloader::cancel()
{
    m_cancelled = true;
}

void CoreDownloader::downloadToFile(const QUrl& url, const QString& destinationPath,
                                    const QString& userAgent, int timeoutMs)
{
    m_cancelled = false;
    if (!url.isValid()) {
        emit finished(false, QStringLiteral("Invalid download URL."));
        return;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::downloadProgress, this,
                     [this](qint64 received, qint64 total) { emit progress(received, total); });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (m_cancelled) {
        reply->abort();
        reply->deleteLater();
        emit finished(false, QStringLiteral("Download cancelled."));
        return;
    }

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        emit finished(false, QStringLiteral("Download timed out."));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        const QString error = reply->errorString();
        reply->deleteLater();
        emit finished(false, error);
        return;
    }

    QFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        reply->deleteLater();
        emit finished(false, file.errorString());
        return;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();
    emit finished(true, {});
}

} // namespace zarya
