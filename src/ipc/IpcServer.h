#pragma once

#include "ipc/IpcMessage.h"

#include <QHash>
#include <QObject>

#include <functional>

class QLocalServer;
class QLocalSocket;

namespace zarya {

class IpcServer : public QObject {
    Q_OBJECT

public:
    explicit IpcServer(QObject* parent = nullptr);
    ~IpcServer() override;

    bool listen(const QString& serverName, QString* errorMessage = nullptr);
    void close();

    using RequestHandler = std::function<void(const IpcEnvelope& request, QLocalSocket* client)>;

    void setRequestHandler(RequestHandler handler);
    void setAuthToken(const QString& token);
    void setAllowedClientSid(const QString& sid);
    void setAllowedClientUid(const QString& uid);

    void sendResponse(QLocalSocket* client, const IpcEnvelope& response);
    void sendEvent(QLocalSocket* client, const IpcEnvelope& event);

signals:
    void clientConnected();
    void clientDisconnected();
    void protocolError(const QString& message);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void processLine(QLocalSocket* client, const QByteArray& line);
    bool isClientAllowed(QLocalSocket* client, QString* reason) const;

    QLocalServer* m_server = nullptr;
    RequestHandler m_handler;
    QString m_authToken;
    QString m_allowedClientSid;
    QString m_allowedClientUid;
    QHash<QLocalSocket*, QByteArray> m_buffers;
};

} // namespace zarya
