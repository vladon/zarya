#pragma once

#include "runtime/core/CoreRuntimeCoordinator.h"
#include "runtime/embedded/singbox/EmbeddedSingBoxRuntimeHost.h"

#include <QDateTime>
#include <QObject>

namespace zarya {

class HelperRuntimeManager : public QObject {
    Q_OBJECT

public:
    explicit HelperRuntimeManager(QObject* parent = nullptr);

    bool isRunning() const;
    QString version() const;
    int abiVersion() const;
    QString loadStatus() const;
    QDateTime startedAt() const;

    bool validateConfig(const QByteArray& configJson, QString* output = nullptr,
                        QString* errorMessage = nullptr);
    bool startTun(const QByteArray& configJson, bool checkBeforeStart,
                  QString* errorMessage = nullptr);
    bool stopTun(QString* errorMessage = nullptr);

signals:
    void logLine(const QString& line);
    void runtimeExited(int exitCode);

private:
    CoreLaunchRequest makeRequest(const QByteArray& configJson) const;

    CoreRuntimeCoordinator m_coordinator;
    EmbeddedSingBoxRuntimeHost m_runtimeHost;
    QDateTime m_startedAt;
};

} // namespace zarya