#pragma once

#include <QObject>
#include <QUrl>
#include <functional>

namespace zarya {

class CoreDownloader : public QObject {
    Q_OBJECT

public:
    explicit CoreDownloader(QObject* parent = nullptr);

    void downloadToFile(const QUrl& url, const QString& destinationPath, const QString& userAgent,
                        int timeoutMs);
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void finished(bool ok, const QString& errorMessage);

private:
    bool m_cancelled = false;
};

} // namespace zarya
