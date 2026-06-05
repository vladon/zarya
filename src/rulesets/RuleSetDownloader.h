#pragma once

#include <QString>
#include <functional>

class QUrl;

namespace zarya {

class RuleSetDownloader {
public:
    explicit RuleSetDownloader(int timeoutMs = 30000);

    bool downloadToFile(const QUrl& url, const QString& destinationPath, const QString& userAgent,
                        QString* errorMessage,
                        const std::function<void(qint64, qint64)>& progressCallback = {},
                        const std::function<bool()>& cancelCallback = {}) const;

private:
    int m_timeoutMs = 30000;
};

} // namespace zarya
