#pragma once

#include "ipc/IpcClient.h"
#include "killswitch/KillSwitchState.h"

#include <QJsonObject>
#include <QObject>
#include <QProcess>

namespace zarya {

enum class HelperConnectionState {
    NotRunning,
    Starting,
    Connected,
    Error,
};

class IHelperServiceManager;

class HelperProcessManager : public QObject {
    Q_OBJECT

public:
    explicit HelperProcessManager(QObject* parent = nullptr);

    void setServiceManager(IHelperServiceManager* serviceManager);

    HelperConnectionState connectionState() const;
    QString statusText() const;
    QString lastError() const;

    QString helperExecutablePath() const;

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
                  bool autoDisableKillSwitchOnFailure = true, QString* errorMessage = nullptr);
    bool stopTun(bool autoDisableKillSwitch = true, QString* errorMessage = nullptr);

    KillSwitchState killSwitchState() const;
    bool killSwitchStatus(QString* errorMessage = nullptr);
    bool killSwitchCheckSupport(QJsonObject* payload = nullptr, QString* errorMessage = nullptr);
    bool killSwitchEnable(const QJsonObject& payload, QString* errorMessage = nullptr);
    bool killSwitchDisable(QString* errorMessage = nullptr);
    bool killSwitchRecover(bool force = true, QString* errorMessage = nullptr);
    static QString recoveryInstructionsText();

signals:
    void connectionStateChanged();
    void helperLogLine(const QString& line);
    void killSwitchStateChanged(const KillSwitchState& state);
    void tunExitedWithKillSwitchActive();

private:
    bool ensureToken(QString* errorMessage);
    QStringList helperArguments() const;
    QString preferredServerName() const;
    bool shouldUseInstalledService() const;

    IHelperServiceManager* m_serviceManager = nullptr;
    QProcess m_helperProcess;
    IpcClient m_client;
    QString m_token;
    HelperConnectionState m_state = HelperConnectionState::NotRunning;
    QString m_lastError;
    QString m_helperVersion;
    bool m_privileged = false;
    KillSwitchState m_killSwitchState;

    void updateKillSwitchState(const QJsonObject& payload);
};

} // namespace zarya
