#include "ipc/IpcMessage.h"

#include <QJsonDocument>

namespace zarya {

IpcEnvelope IpcEnvelope::fromJson(const QJsonObject& object)
{
    IpcEnvelope envelope;
    envelope.version = object.value(QStringLiteral("version")).toInt(1);
    envelope.id = object.value(QStringLiteral("id")).toString();
    envelope.type = object.value(QStringLiteral("type")).toString();
    envelope.command = object.value(QStringLiteral("command")).toString();
    envelope.event = object.value(QStringLiteral("event")).toString();
    envelope.token = object.value(QStringLiteral("token")).toString();
    envelope.ok = object.value(QStringLiteral("ok")).toBool();
    envelope.payload = object.value(QStringLiteral("payload")).toObject();
    return envelope;
}

QJsonObject IpcEnvelope::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), version);
    if (!id.isEmpty()) {
        object.insert(QStringLiteral("id"), id);
    }
    if (!type.isEmpty()) {
        object.insert(QStringLiteral("type"), type);
    }
    if (!command.isEmpty()) {
        object.insert(QStringLiteral("command"), command);
    }
    if (!event.isEmpty()) {
        object.insert(QStringLiteral("event"), event);
    }
    if (!token.isEmpty()) {
        object.insert(QStringLiteral("token"), token);
    }
    if (type == ipcTypeResponse()) {
        object.insert(QStringLiteral("ok"), ok);
    }
    if (!payload.isEmpty()) {
        object.insert(QStringLiteral("payload"), payload);
    }
    return object;
}

QString ipcCommandHello() { return QStringLiteral("hello"); }
QString ipcCommandStatus() { return QStringLiteral("status"); }
QString ipcCommandCheckSupport() { return QStringLiteral("checkSupport"); }
QString ipcCommandValidateConfig() { return QStringLiteral("validateConfig"); }
QString ipcCommandStartTun() { return QStringLiteral("startTun"); }
QString ipcCommandStopTun() { return QStringLiteral("stopTun"); }
QString ipcCommandGetLogs() { return QStringLiteral("getLogs"); }
QString ipcCommandShutdownHelper() { return QStringLiteral("shutdownHelper"); }

QString ipcTypeRequest() { return QStringLiteral("request"); }
QString ipcTypeResponse() { return QStringLiteral("response"); }
QString ipcTypeEvent() { return QStringLiteral("event"); }

QString ipcEventLog() { return QStringLiteral("log"); }
QString ipcEventRuntimeExited() { return QStringLiteral("runtimeExited"); }

} // namespace zarya
