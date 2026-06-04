#include "ipc/IpcServer.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>

namespace zarya {

IpcServer::IpcServer(QObject* parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

IpcServer::~IpcServer()
{
    close();
}

bool IpcServer::listen(const QString& serverName, QString* errorMessage)
{
    QLocalServer::removeServer(serverName);
    if (!m_server->listen(serverName)) {
        if (errorMessage) {
            *errorMessage = m_server->errorString();
        }
        return false;
    }
    return true;
}

void IpcServer::close()
{
    for (auto it = m_buffers.begin(); it != m_buffers.end(); ++it) {
        it.key()->disconnect(this);
        it.key()->close();
    }
    m_buffers.clear();
    m_server->close();
}

void IpcServer::setRequestHandler(RequestHandler handler)
{
    m_handler = std::move(handler);
}

void IpcServer::setAuthToken(const QString& token)
{
    m_authToken = token;
}

void IpcServer::sendResponse(QLocalSocket* client, const IpcEnvelope& response)
{
    if (!client) {
        return;
    }
    IpcEnvelope envelope = response;
    envelope.type = ipcTypeResponse();
    const QByteArray line = QJsonDocument(envelope.toJson()).toJson(QJsonDocument::Compact)
                          + '\n';
    client->write(line);
    client->flush();
}

void IpcServer::sendEvent(QLocalSocket* client, const IpcEnvelope& event)
{
    if (!client) {
        return;
    }
    IpcEnvelope envelope = event;
    envelope.type = ipcTypeEvent();
    const QByteArray line = QJsonDocument(envelope.toJson()).toJson(QJsonDocument::Compact)
                          + '\n';
    client->write(line);
    client->flush();
}

void IpcServer::onNewConnection()
{
    while (QLocalSocket* client = m_server->nextPendingConnection()) {
        m_buffers.insert(client, {});
        connect(client, &QLocalSocket::readyRead, this, &IpcServer::onReadyRead);
        connect(client, &QLocalSocket::disconnected, this, [this, client]() {
            m_buffers.remove(client);
            client->deleteLater();
            emit clientDisconnected();
        });
        emit clientConnected();
    }
}

void IpcServer::onReadyRead()
{
    auto* client = qobject_cast<QLocalSocket*>(sender());
    if (!client) {
        return;
    }
    m_buffers[client].append(client->readAll());
    int index = -1;
    while ((index = m_buffers[client].indexOf('\n')) >= 0) {
        const QByteArray line = m_buffers[client].left(index);
        m_buffers[client].remove(0, index + 1);
        processLine(client, line);
    }
}

void IpcServer::processLine(QLocalSocket* client, const QByteArray& line)
{
    if (line.trimmed().isEmpty()) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit protocolError(parseError.errorString());
        return;
    }

    const IpcEnvelope request = IpcEnvelope::fromJson(document.object());
    if (request.type != ipcTypeRequest()) {
        return;
    }

    if (!m_authToken.isEmpty() && request.token != m_authToken) {
        IpcEnvelope response;
        response.id = request.id;
        response.ok = false;
        response.payload = QJsonObject{
            {QStringLiteral("error"), QStringLiteral("IPC auth failed: invalid token")}};
        sendResponse(client, response);
        return;
    }

    if (m_handler) {
        m_handler(request, client);
    }
}

} // namespace zarya
