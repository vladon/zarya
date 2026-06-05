#include "rulesets/RuleSetDownloader.h"

#include "geodata/GeoDataDownloader.h"

namespace zarya {

RuleSetDownloader::RuleSetDownloader(int timeoutMs)
    : m_timeoutMs(timeoutMs)
{
}

bool RuleSetDownloader::downloadToFile(
    const QUrl& url, const QString& destinationPath, const QString& userAgent,
    QString* errorMessage, const std::function<void(qint64, qint64)>& progressCallback,
    const std::function<bool()>& cancelCallback) const
{
    GeoDataDownloader downloader(m_timeoutMs);
    return downloader.downloadToFile(url, destinationPath, userAgent, errorMessage,
                                     progressCallback, cancelCallback);
}

} // namespace zarya
