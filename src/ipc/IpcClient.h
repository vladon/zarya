#pragma once

#include "ipc/IpcMessage.h"

#include <QObject>

#include <functional>

class QLocalSocket;

namespace zarya {

class IpcClient : public QObject {
    Q_OBJECT

public:
    explicit IpcClient(QObject* parent = nullptr);
    ~IpcClient() override;

    void setAuthToken(const QString& token);

    bool connectToServer(const QString& serverName, QString* errorMessage = nullptr);
    void disconnectFromServer();
    bool isConnected() const;

    bool sendRequest(const IpcEnvelope& request, IpcEnvelope* response,
                     QString* errorMessage = nullptr, int timeoutMs = 30000);

    using EventHandler = std::function<void(const IpcEnvelope& event)>;

    void setEventHandler(EventHandler handler);

signals:
    void disconnected();
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    bool processBuffer(QString* errorMessage);

    QLocalSocket* m_socket = nullptr;
    QString m_authToken;
    QByteArray m_buffer;
    EventHandler m_eventHandler;
};

} // namespace zarya
