#pragma once

#include "helper/HelperPathPolicy.h"
#include "helper/HelperRuntimeManager.h"
#include "ipc/IpcServer.h"
#include "killswitch/KillSwitchManager.h"

#include <QObject>

class QLocalSocket;

namespace zarya {

class HelperCommandServer : public QObject {
    Q_OBJECT

public:
    explicit HelperCommandServer(QObject* parent = nullptr);

    bool start(const QString& serverName, const QString& authToken,
               const HelperPathPolicy& pathPolicy, QString* errorMessage = nullptr);
    void shutdown();

signals:
    void logLine(const QString& line);

private:
    void handleRequest(const IpcEnvelope& request, QLocalSocket* client);
    void sendOk(QLocalSocket* client, const IpcEnvelope& request, const QJsonObject& payload);
    void sendError(QLocalSocket* client, const IpcEnvelope& request, const QString& error);
    void broadcastLog(const QString& line);

    IpcServer m_server;
    HelperRuntimeManager m_runtime;
    KillSwitchManager m_killSwitch;
    HelperPathPolicy m_pathPolicy;
    QLocalSocket* m_activeClient = nullptr;
};

} // namespace zarya
