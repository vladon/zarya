#include "ipc/IpcClient.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>
#include <QTimer>
#include <QUuid>

namespace zarya {

IpcClient::IpcClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::readyRead, this, &IpcClient::onReadyRead);
    connect(m_socket, &QLocalSocket::disconnected, this, &IpcClient::onDisconnected);
}

IpcClient::~IpcClient()
{
    disconnectFromServer();
}

void IpcClient::setAuthToken(const QString& token)
{
    m_authToken = token;
}

bool IpcClient::connectToServer(const QString& serverName, QString* errorMessage)
{
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        return true;
    }
    m_socket->connectToServer(serverName);
    if (!m_socket->waitForConnected(3000)) {
        if (errorMessage) {
            *errorMessage = m_socket->errorString();
        }
        return false;
    }
    return true;
}

void IpcClient::disconnectFromServer()
{
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
        if (m_socket->state() != QLocalSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
    m_buffer.clear();
}

bool IpcClient::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

bool IpcClient::sendRequest(const IpcEnvelope& request, IpcEnvelope* response,
                            QString* errorMessage, int timeoutMs)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Not connected to helper.");
        }
        return false;
    }

    IpcEnvelope outgoing = request;
    outgoing.type = ipcTypeRequest();
    outgoing.version = 1;
    if (outgoing.id.isEmpty()) {
        outgoing.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    outgoing.token = m_authToken;

    const QByteArray line =
        QJsonDocument(outgoing.toJson()).toJson(QJsonDocument::Compact) + '\n';
    m_socket->write(line);
    if (!m_socket->waitForBytesWritten(3000)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write IPC request.");
        }
        return false;
    }

    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);

    IpcEnvelope localResponse;
    bool gotResponse = false;

    while (timer.isActive()) {
        if (m_socket->waitForReadyRead(100)) {
            m_buffer.append(m_socket->readAll());
        }

        int newlineIndex = -1;
        while ((newlineIndex = m_buffer.indexOf('\n')) >= 0) {
            const QByteArray lineBytes = m_buffer.left(newlineIndex);
            m_buffer.remove(0, newlineIndex + 1);
            if (lineBytes.trimmed().isEmpty()) {
                continue;
            }

            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(lineBytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                continue;
            }

            const IpcEnvelope envelope = IpcEnvelope::fromJson(document.object());
            if (envelope.type == ipcTypeEvent()) {
                if (m_eventHandler) {
                    m_eventHandler(envelope);
                }
                continue;
            }
            if (envelope.type == ipcTypeResponse() && envelope.id == outgoing.id) {
                localResponse = envelope;
                gotResponse = true;
                break;
            }
        }

        if (gotResponse) {
            break;
        }
        QCoreApplication::processEvents();
    }

    if (!gotResponse) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("IPC request timed out.");
        }
        return false;
    }

    if (response) {
        *response = localResponse;
    }
    if (!localResponse.ok) {
        if (errorMessage) {
            const QString helperError = localResponse.payload.value(QStringLiteral("error")).toString();
            *errorMessage = helperError.isEmpty()
                                 ? QStringLiteral("Helper rejected the request.")
                                 : helperError;
        }
        return false;
    }
    return true;
}

void IpcClient::setEventHandler(EventHandler handler)
{
    m_eventHandler = std::move(handler);
}

void IpcClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    QString error;
    processBuffer(&error);
    if (!error.isEmpty()) {
        emit errorOccurred(error);
    }
}

void IpcClient::onDisconnected()
{
    emit disconnected();
}

bool IpcClient::processBuffer(QString* errorMessage)
{
    bool consumed = false;
    int newlineIndex = -1;
    while ((newlineIndex = m_buffer.indexOf('\n')) >= 0) {
        consumed = true;
        const QByteArray lineBytes = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);
        if (lineBytes.trimmed().isEmpty()) {
            continue;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(lineBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            if (errorMessage) {
                *errorMessage = parseError.errorString();
            }
            continue;
        }
        const IpcEnvelope envelope = IpcEnvelope::fromJson(document.object());
        if (envelope.type == ipcTypeEvent() && m_eventHandler) {
            m_eventHandler(envelope);
        }
    }
    return consumed;
}

} // namespace zarya
