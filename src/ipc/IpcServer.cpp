#include "ipc/IpcServer.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>

#if defined(Q_OS_LINUX)
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

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

void IpcServer::setAllowedClientSid(const QString& sid)
{
    m_allowedClientSid = sid.trimmed();
}

void IpcServer::setAllowedClientUid(const QString& uid)
{
    m_allowedClientUid = uid.trimmed();
}

bool IpcServer::isClientAllowed(QLocalSocket* client, QString* reason) const
{
    if (m_allowedClientSid.isEmpty() && m_allowedClientUid.isEmpty()) {
        return true;
    }
    if (!client) {
        if (reason) {
            *reason = QStringLiteral("missing client socket");
        }
        return false;
    }

#if defined(Q_OS_LINUX)
    if (!m_allowedClientUid.isEmpty()) {
        struct ucred cred{};
        socklen_t len = sizeof(cred);
        if (getsockopt(client->socketDescriptor(), SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0) {
            if (reason) {
                *reason = QStringLiteral("unable to read peer credentials");
            }
            return false;
        }
        if (QString::number(static_cast<qulonglong>(cred.uid)) != m_allowedClientUid) {
            if (reason) {
                *reason = QStringLiteral("client UID is not authorized");
            }
            return false;
        }
        return true;
    }
#endif

#if defined(Q_OS_WIN)
    if (!m_allowedClientSid.isEmpty()) {
        // Windows local socket peer SID verification is limited in 0.28; token auth remains required.
        Q_UNUSED(client);
        return true;
    }
#endif

    Q_UNUSED(client);
    if (reason) {
        *reason = QStringLiteral("client identity verification is not available on this platform");
    }
    return m_allowedClientSid.isEmpty() && m_allowedClientUid.isEmpty();
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

    QString clientReason;
    if (!isClientAllowed(client, &clientReason)) {
        IpcEnvelope response;
        response.id = request.id;
        response.ok = false;
        response.payload =
            QJsonObject{{QStringLiteral("error"),
                         QStringLiteral("IPC auth failed: %1").arg(clientReason)}};
        sendResponse(client, response);
        return;
    }

    if (m_handler) {
        m_handler(request, client);
    }
}

} // namespace zarya
