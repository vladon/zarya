#pragma once

#include <QJsonObject>
#include <QString>

namespace zarya {

struct IpcEnvelope {
    int version = 1;
    QString id;
    QString type;
    QString command;
    QString event;
    QString token;
    bool ok = false;
    QJsonObject payload;

    static IpcEnvelope fromJson(const QJsonObject& object);
    QJsonObject toJson() const;
};

QString ipcCommandHello();
QString ipcCommandStatus();
QString ipcCommandCheckSupport();
QString ipcCommandValidateConfig();
QString ipcCommandStartTun();
QString ipcCommandStopTun();
QString ipcCommandGetLogs();
QString ipcCommandShutdownHelper();

QString ipcTypeRequest();
QString ipcTypeResponse();
QString ipcTypeEvent();

QString ipcEventLog();
QString ipcEventRuntimeExited();

} // namespace zarya
