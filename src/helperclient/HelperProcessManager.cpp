#include "helperclient/HelperProcessManager.h"

#include "ipc/IpcMessage.h"
#include "ipc/IpcTransport.h"
#include "killswitch/KillSwitchManager.h"
#include "killswitch/KillSwitchMode.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "storage/HelperSession.h"

#include <QFileInfo>
#include <QJsonObject>

namespace zarya {

HelperProcessManager::HelperProcessManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_helperProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus status) {
                Q_UNUSED(exitCode);
                Q_UNUSED(status);
                m_client.disconnectFromServer();
                m_state = HelperConnectionState::NotRunning;
                emit connectionStateChanged();
            });

    m_client.setEventHandler([this](const IpcEnvelope& event) {
        if (event.event == ipcEventLog()) {
            emit helperLogLine(event.payload.value(QStringLiteral("line")).toString());
        }
        if (event.event == ipcEventRuntimeExited()) {
            emit helperLogLine(QStringLiteral("helper: sing-box exited (code %1)")
                                   .arg(event.payload.value(QStringLiteral("exitCode")).toInt()));
            if (m_killSwitchState.status == KillSwitchStatus::Enabled) {
                emit tunExitedWithKillSwitchActive();
            }
        }
        if (event.event == ipcEventKillSwitchState()) {
            updateKillSwitchState(event.payload);
        }
    });
}

void HelperProcessManager::updateKillSwitchState(const QJsonObject& payload)
{
    QJsonObject stateObject = payload.value(QStringLiteral("killSwitch")).toObject();
    if (stateObject.isEmpty() && payload.contains(QStringLiteral("status"))) {
        stateObject = payload;
    }
    if (stateObject.isEmpty()) {
        return;
    }
    const QString statusText = stateObject.value(QStringLiteral("status")).toString();
    if (statusText == QStringLiteral("enabled")) {
        m_killSwitchState.status = KillSwitchStatus::Enabled;
    } else if (statusText == QStringLiteral("disabled")) {
        m_killSwitchState.status = KillSwitchStatus::Disabled;
    } else if (statusText == QStringLiteral("failed")) {
        m_killSwitchState.status = KillSwitchStatus::Failed;
    } else if (statusText == QStringLiteral("needs-recovery")) {
        m_killSwitchState.status = KillSwitchStatus::NeedsRecovery;
    } else if (statusText == QStringLiteral("unsupported")) {
        m_killSwitchState.status = KillSwitchStatus::Unsupported;
    }
    m_killSwitchState.backend = stateObject.value(QStringLiteral("backend")).toString();
    m_killSwitchState.lastError = stateObject.value(QStringLiteral("lastError")).toString();
    m_killSwitchState.recoveryMarkerPresent =
        stateObject.value(QStringLiteral("recoveryMarkerPresent")).toBool();
    m_killSwitchState.privileged = stateObject.value(QStringLiteral("privileged")).toBool();
    m_killSwitchState.supported = stateObject.value(QStringLiteral("supported")).toBool();
    m_killSwitchState.mode =
        killSwitchModeFromString(stateObject.value(QStringLiteral("mode")).toString());
    emit killSwitchStateChanged(m_killSwitchState);
}

KillSwitchState HelperProcessManager::killSwitchState() const
{
    return m_killSwitchState;
}

QString HelperProcessManager::recoveryInstructionsText()
{
    return KillSwitchManager::recoveryInstructionsForPlatform();
}

HelperConnectionState HelperProcessManager::connectionState() const
{
    return m_state;
}

QString HelperProcessManager::statusText() const
{
    switch (m_state) {
    case HelperConnectionState::NotRunning:
        return QStringLiteral("Not running");
    case HelperConnectionState::Starting:
        return QStringLiteral("Starting…");
    case HelperConnectionState::Connected:
        if (!m_helperVersion.isEmpty()) {
            return QStringLiteral("Connected (%1, privileged: %2)")
                .arg(m_helperVersion, m_privileged ? QStringLiteral("yes")
                                                 : QStringLiteral("no"));
        }
        return QStringLiteral("Connected");
    case HelperConnectionState::Error:
        return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString HelperProcessManager::lastError() const
{
    return m_lastError;
}

IpcClient* HelperProcessManager::client()
{
    return &m_client;
}

bool HelperProcessManager::ensureToken(QString* errorMessage)
{
    if (!m_token.isEmpty()) {
        return true;
    }
    m_token = HelperSession::ensureSessionToken(errorMessage);
    return !m_token.isEmpty();
}

QString HelperProcessManager::helperExecutablePath() const
{
    return AppPaths::resolvedHelperPath();
}

QStringList HelperProcessManager::helperArguments() const
{
    AppPaths::initialize(AppPaths::isPortableMode());
    QString allowedCoreDir = AppPaths::singBoxCoreDir();
    const QString configured = AppSettings::instance().singBoxExecutablePath().trimmed();
    if (!configured.isEmpty()) {
        const QFileInfo info(configured);
        if (info.exists()) {
            allowedCoreDir = info.absolutePath();
        }
    }
    return {QStringLiteral("--dev"),
            QStringLiteral("--token-file"),
            HelperSession::tokenFilePath(),
            QStringLiteral("--allowed-runtime-dir"),
            AppPaths::runtimeDir(),
            QStringLiteral("--allowed-core-dir"),
            allowedCoreDir};
}

bool HelperProcessManager::startHelperDevMode(QString* errorMessage)
{
    if (!ensureToken(errorMessage)) {
        m_state = HelperConnectionState::Error;
        m_lastError = errorMessage ? *errorMessage : QString();
        emit connectionStateChanged();
        return false;
    }

    if (m_helperProcess.state() != QProcess::NotRunning) {
        return connectToHelper(errorMessage);
    }

    const QString helperPath = helperExecutablePath();
    if (!QFileInfo::exists(helperPath)) {
        m_lastError = QStringLiteral("zarya-helper not found: %1").arg(helperPath);
        m_state = HelperConnectionState::Error;
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        emit connectionStateChanged();
        return false;
    }

    m_state = HelperConnectionState::Starting;
    emit connectionStateChanged();

    m_helperProcess.setProgram(helperPath);
    m_helperProcess.setArguments(helperArguments());
    m_helperProcess.start();
    if (!m_helperProcess.waitForStarted(5000)) {
        m_lastError = m_helperProcess.errorString();
        m_state = HelperConnectionState::Error;
        if (errorMessage) {
            *errorMessage = m_lastError;
        }
        emit connectionStateChanged();
        return false;
    }

    return connectToHelper(errorMessage);
}

bool HelperProcessManager::connectToHelper(QString* errorMessage)
{
    if (!ensureToken(errorMessage)) {
        return false;
    }

    m_client.setAuthToken(m_token);
    const QString serverName = IpcTransport::defaultServerName();
    if (!m_client.connectToServer(serverName, errorMessage)) {
        m_lastError = errorMessage ? *errorMessage : QString();
        m_state = HelperConnectionState::Error;
        emit connectionStateChanged();
        return false;
    }

    QJsonObject helloPayload;
    if (!hello(&helloPayload, errorMessage)) {
        m_state = HelperConnectionState::Error;
        emit connectionStateChanged();
        return false;
    }

    m_helperVersion = helloPayload.value(QStringLiteral("helperVersion")).toString();
    m_privileged = helloPayload.value(QStringLiteral("privileged")).toBool();
    m_state = HelperConnectionState::Connected;
    emit connectionStateChanged();
    return true;
}

void HelperProcessManager::disconnectFromHelper()
{
    m_client.disconnectFromServer();
    if (m_helperProcess.state() != QProcess::NotRunning) {
        IpcEnvelope request;
        request.command = ipcCommandShutdownHelper();
        IpcEnvelope response;
        m_client.sendRequest(request, &response, nullptr, 2000);
        m_helperProcess.waitForFinished(2000);
        if (m_helperProcess.state() != QProcess::NotRunning) {
            m_helperProcess.kill();
            m_helperProcess.waitForFinished(1000);
        }
    }
    m_state = HelperConnectionState::NotRunning;
    emit connectionStateChanged();
}

bool HelperProcessManager::isHelperProcessRunning() const
{
    return m_helperProcess.state() != QProcess::NotRunning;
}

bool HelperProcessManager::hello(QJsonObject* payload, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandHello();
    request.payload = QJsonObject{{QStringLiteral("clientName"), QStringLiteral("Zarya")},
                                  {QStringLiteral("clientVersion"),
                                   PackagingInfo::versionString()}};
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage)) {
        return false;
    }
    if (payload) {
        *payload = response.payload;
    }
    return true;
}

bool HelperProcessManager::status(QJsonObject* payload, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandStatus();
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage)) {
        return false;
    }
    if (payload) {
        *payload = response.payload;
    }
    return true;
}

bool HelperProcessManager::validateConfig(const QString& singBoxPath, const QString& configPath,
                                          QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandValidateConfig();
    request.payload =
        QJsonObject{{QStringLiteral("singBoxPath"), singBoxPath},
                    {QStringLiteral("configPath"), configPath}};
    IpcEnvelope response;
    return m_client.sendRequest(request, &response, errorMessage);
}

bool HelperProcessManager::startTun(const QString& singBoxPath, const QString& configPath,
                                    bool autoDisableKillSwitchOnFailure, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandStartTun();
    request.payload = QJsonObject{
        {QStringLiteral("singBoxPath"), singBoxPath},
        {QStringLiteral("configPath"), configPath},
        {QStringLiteral("workingDirectory"), QFileInfo(singBoxPath).absolutePath()},
        {QStringLiteral("checkBeforeStart"), true},
        {QStringLiteral("autoDisableKillSwitchOnFailure"), autoDisableKillSwitchOnFailure}};
    IpcEnvelope response;
    return m_client.sendRequest(request, &response, errorMessage, 120000);
}

bool HelperProcessManager::stopTun(bool autoDisableKillSwitch, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandStopTun();
    request.payload =
        QJsonObject{{QStringLiteral("autoDisableKillSwitch"), autoDisableKillSwitch}};
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage, 30000)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    return true;
}

bool HelperProcessManager::killSwitchStatus(QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandKillSwitchStatus();
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    return true;
}

bool HelperProcessManager::killSwitchCheckSupport(QJsonObject* payload, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandKillSwitchCheckSupport();
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    if (payload) {
        *payload = response.payload;
    }
    return true;
}

bool HelperProcessManager::killSwitchEnable(const QJsonObject& payload, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandKillSwitchEnable();
    request.payload = payload;
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage, 60000)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    return true;
}

bool HelperProcessManager::killSwitchDisable(QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandKillSwitchDisable();
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage, 60000)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    return true;
}

bool HelperProcessManager::killSwitchRecover(bool force, QString* errorMessage)
{
    IpcEnvelope request;
    request.command = ipcCommandKillSwitchRecover();
    request.payload = QJsonObject{{QStringLiteral("force"), force}};
    IpcEnvelope response;
    if (!m_client.sendRequest(request, &response, errorMessage, 60000)) {
        return false;
    }
    updateKillSwitchState(response.payload);
    return true;
}

} // namespace zarya
