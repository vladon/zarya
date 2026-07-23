#pragma once

#include "geodata/GeoDataFileStatus.h"

#include <QByteArray>
#include <functional>

class QUrl;

namespace zarya {

struct GeoDataDownloadResult {
    bool success = false;
    QByteArray body;
    QString errorMessage;
    int httpStatusCode = 0;
};

class GeoDataDownloader {
public:
    /// timeoutMs is a soft overall limit; large-file downloads use at least 5 minutes.
    explicit GeoDataDownloader(int timeoutMs = 30000);

    GeoDataDownloadResult download(
        const QUrl& url, const QString& userAgent,
        const std::function<void(qint64, qint64)>& progressCallback = {},
        const std::function<bool()>& cancelCallback = {}) const;

    bool downloadToFile(const QUrl& url, const QString& destinationPath, const QString& userAgent,
                        QString* errorMessage,
                        const std::function<void(qint64, qint64)>& progressCallback = {},
                        const std::function<bool()>& cancelCallback = {}) const;

private:
    int m_timeoutMs = 30000;
};

} // namespace zarya
