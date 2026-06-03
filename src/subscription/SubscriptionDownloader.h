#pragma once

#include <QByteArray>
#include <QString>

namespace zarya {

struct SubscriptionDownloadResult {
    bool success = false;
    int httpStatusCode = 0;
    QByteArray body;
    QString finalUrl;
    QString errorMessage;
};

class SubscriptionDownloader {
public:
    explicit SubscriptionDownloader(int timeoutMs = 20000);

    SubscriptionDownloadResult download(const QString& url, const QString& userAgent) const;

private:
    int m_timeoutMs;
};

} // namespace zarya
