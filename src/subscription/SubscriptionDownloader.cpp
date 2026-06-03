#include "subscription/SubscriptionDownloader.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace zarya {

SubscriptionDownloader::SubscriptionDownloader(int timeoutMs)
    : m_timeoutMs(timeoutMs)
{
}

SubscriptionDownloadResult SubscriptionDownloader::download(const QString& url,
                                                          const QString& userAgent) const
{
    SubscriptionDownloadResult result;
    const QUrl requestUrl(url.trimmed());
    if (!requestUrl.isValid() || requestUrl.scheme().isEmpty()) {
        result.errorMessage = QStringLiteral("Invalid subscription URL.");
        return result;
    }

    if (requestUrl.isLocalFile()) {
        QFile file(requestUrl.toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) {
            result.errorMessage = file.errorString();
            return result;
        }
        result.body = file.readAll();
        result.finalUrl = requestUrl.toString();
        result.httpStatusCode = 200;
        if (result.body.isEmpty()) {
            result.errorMessage = QStringLiteral("Subscription file is empty.");
            return result;
        }
        result.success = true;
        return result;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(m_timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        result.errorMessage = QStringLiteral("Subscription download timed out.");
        reply->deleteLater();
        return result;
    }

    result.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.finalUrl = reply->url().toString();
    result.body = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMessage = reply->errorString();
        reply->deleteLater();
        return result;
    }

    reply->deleteLater();

    if (result.httpStatusCode < 200 || result.httpStatusCode >= 300) {
        result.errorMessage =
            QStringLiteral("HTTP error %1").arg(result.httpStatusCode);
        return result;
    }

    if (result.body.isEmpty()) {
        result.errorMessage = QStringLiteral("Subscription response body is empty.");
        return result;
    }

    result.success = true;
    return result;
}

} // namespace zarya
