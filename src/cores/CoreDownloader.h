#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

class QNetworkReply;

namespace zarya {

class CoreDownloader : public QObject {
    Q_OBJECT

public:
    explicit CoreDownloader(QObject* parent = nullptr);

    /// Synchronous download. Emits progress during transfer and finished when done.
    /// Returns true on success. Prefer the return value over waiting on finished —
    /// finished is emitted before this function returns (same thread).
    bool downloadToFile(const QUrl& url, const QString& destinationPath, const QString& userAgent,
                        int timeoutMs, QString* errorMessage = nullptr);
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void finished(bool ok, const QString& errorMessage);

private:
    bool m_cancelled = false;
    QPointer<QNetworkReply> m_activeReply;
};

} // namespace zarya
