#pragma once

#include "ipc/IpcClient.h"

#include <QObject>
#include <QProcess>

namespace zarya {

enum class HelperConnectionState {
    NotRunning,
    Starting,
    Connected,
    Error,
};

class HelperProcessManager : public QObject {
    Q_OBJECT

public:
    explicit HelperProcessManager(QObject* parent = nullptr);

    HelperConnectionState connectionState() const;
    QString statusText() const;
    QString lastError() const;

    bool startHelperDevMode(QString* errorMessage = nullptr);
    bool connectToHelper(QString* errorMessage = nullptr);
    void disconnectFromHelper();
    bool isHelperProcessRunning() const;

    IpcClient* client();

    bool hello(QJsonObject* payload = nullptr, QString* errorMessage = nullptr);
    bool status(QJsonObject* payload = nullptr, QString* errorMessage = nullptr);
    bool validateConfig(const QString& singBoxPath, const QString& configPath,
                        QString* errorMessage = nullptr);
    bool startTun(const QString& singBoxPath, const QString& configPath,
                  QString* errorMessage = nullptr);
    bool stopTun(QString* errorMessage = nullptr);

signals:
    void connectionStateChanged();
    void helperLogLine(const QString& line);

private:
    bool ensureToken(QString* errorMessage);
    QString helperExecutablePath() const;
    QStringList helperArguments() const;

    QProcess m_helperProcess;
    IpcClient m_client;
    QString m_token;
    HelperConnectionState m_state = HelperConnectionState::NotRunning;
    QString m_lastError;
    QString m_helperVersion;
    bool m_privileged = false;
};

} // namespace zarya
