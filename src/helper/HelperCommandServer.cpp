#include "helper/HelperCommandServer.h"

#include "ipc/IpcMessage.h"
#include "ipc/IpcTransport.h"
#include "packaging/PackagingInfo.h"
#include "platform/PlatformPrivilege.h"
#include "runtime/singbox/SingBoxTunSupportChecker.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QLocalSocket>

namespace zarya {

HelperCommandServer::HelperCommandServer(QObject* parent)
    : QObject(parent)
{
    m_runtime.setPathPolicy(&m_pathPolicy);
    connect(&m_runtime, &HelperRuntimeManager::logLine, this, &HelperCommandServer::logLine);
    connect(&m_runtime, &HelperRuntimeManager::logLine, this, &HelperCommandServer::broadcastLog);
    connect(&m_runtime, &HelperRuntimeManager::runtimeExited, this, [this](int exitCode) {
        if (!m_activeClient) {
            return;
        }
        IpcEnvelope event;
        event.event = ipcEventRuntimeExited();
        event.payload = QJsonObject{{QStringLiteral("exitCode"), exitCode}};
        m_server.sendEvent(m_activeClient, event);

        const KillSwitchState killSwitch = m_killSwitch.state();
        if (killSwitch.status == KillSwitchStatus::Enabled) {
            emit logLine(QStringLiteral(
                "helper: sing-box exited unexpectedly; kill switch remains active"));
            IpcEnvelope killSwitchEvent;
            killSwitchEvent.event = ipcEventKillSwitchState();
            killSwitchEvent.payload =
                QJsonObject{{QStringLiteral("killSwitch"),
                             KillSwitchManager::stateToJson(killSwitch)},
                            {QStringLiteral("unexpectedExit"), true}};
            m_server.sendEvent(m_activeClient, killSwitchEvent);
        }
    });
    connect(&m_killSwitch, &KillSwitchManager::logLine, this, &HelperCommandServer::logLine);
    connect(&m_killSwitch, &KillSwitchManager::logLine, this, &HelperCommandServer::broadcastLog);
    connect(&m_killSwitch, &KillSwitchManager::stateChanged, this,
            [this](const KillSwitchState& state) {
                if (!m_activeClient) {
                    return;
                }
                IpcEnvelope event;
                event.event = ipcEventKillSwitchState();
                event.payload =
                    QJsonObject{{QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(state)}};
                m_server.sendEvent(m_activeClient, event);
            });
}

bool HelperCommandServer::start(const QString& serverName, const QString& authToken,
                                const HelperPathPolicy& pathPolicy, QString* errorMessage)
{
    m_pathPolicy = pathPolicy;
    m_server.setAuthToken(authToken);
    m_server.setRequestHandler([this](const IpcEnvelope& request, QLocalSocket* client) {
        handleRequest(request, client);
    });

    if (!m_server.listen(serverName, errorMessage)) {
        return false;
    }

    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    m_killSwitch.refreshStartupState(privileges.elevated);

    emit logLine(QStringLiteral("helper: listening on %1").arg(serverName));
    return true;
}

void HelperCommandServer::shutdown()
{
    m_runtime.stopTun();
    m_killSwitch.disable(nullptr);
    m_server.close();
}

void HelperCommandServer::handleRequest(const IpcEnvelope& request, QLocalSocket* client)
{
    m_activeClient = client;

    if (request.command == ipcCommandHello()) {
        const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
        const TunSupportResult support = SingBoxTunSupportChecker::check(QString());
        QJsonArray warnings;
        for (const QString& warning : support.warnings) {
            warnings.append(warning);
        }
        sendOk(client, request,
               QJsonObject{{QStringLiteral("helperVersion"), PackagingInfo::versionString()},
                           {QStringLiteral("platform"), support.platform},
                           {QStringLiteral("privileged"), privileges.elevated},
                           {QStringLiteral("tunSupported"), support.supported},
                           {QStringLiteral("warnings"), warnings}});
        return;
    }

    if (request.command == ipcCommandStatus()) {
        sendOk(client, request,
               QJsonObject{{QStringLiteral("running"), m_runtime.isRunning()},
                           {QStringLiteral("runtime"), QStringLiteral("sing-box-tun")},
                           {QStringLiteral("pid"), m_runtime.isRunning() ? m_runtime.processId() : 0},
                           {QStringLiteral("configPath"), m_runtime.configPath()},
                           {QStringLiteral("startedAt"),
                            m_runtime.startedAt().toString(Qt::ISODate)},
                           {QStringLiteral("killSwitch"),
                            KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandKillSwitchStatus()) {
        sendOk(client, request,
               QJsonObject{
                   {QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandKillSwitchCheckSupport()) {
        const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
        const KillSwitchState support = m_killSwitch.checkSupport(privileges.elevated);
        sendOk(client, request, KillSwitchManager::stateToJson(support));
        return;
    }

    if (request.command == ipcCommandKillSwitchEnable()) {
        const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
        const KillSwitchRuleSet rules = KillSwitchManager::ruleSetFromJson(request.payload);
        QString error;
        if (!m_killSwitch.enable(rules, privileges.elevated, &error)) {
            sendError(client, request, error);
            return;
        }
        sendOk(client, request,
               QJsonObject{
                   {QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandKillSwitchDisable()) {
        QString error;
        if (!m_killSwitch.disable(&error)) {
            sendError(client, request, error);
            return;
        }
        sendOk(client, request,
               QJsonObject{
                   {QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandKillSwitchRecover()) {
        const bool force = request.payload.value(QStringLiteral("force")).toBool(true);
        QString error;
        if (!m_killSwitch.recover(force, &error)) {
            sendError(client, request, error);
            return;
        }
        sendOk(client, request,
               QJsonObject{
                   {QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandCheckSupport()) {
        const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
        const TunSupportResult support =
            SingBoxTunSupportChecker::check(request.payload.value(QStringLiteral("singBoxPath"))
                                                .toString());
        QJsonArray warnings;
        for (const QString& warning : support.warnings) {
            warnings.append(warning);
        }
        sendOk(client, request,
               QJsonObject{{QStringLiteral("privileged"), privileges.elevated},
                           {QStringLiteral("tunSupported"), support.supported},
                           {QStringLiteral("warnings"), warnings}});
        return;
    }

    if (request.command == ipcCommandValidateConfig()) {
        const QString singBoxPath = request.payload.value(QStringLiteral("singBoxPath")).toString();
        const QString configPath = request.payload.value(QStringLiteral("configPath")).toString();
        QString output;
        QString error;
        if (!m_runtime.validateConfig(singBoxPath, configPath, &output, &error)) {
            sendError(client, request, error);
            return;
        }
        sendOk(client, request, QJsonObject{{QStringLiteral("output"), output}});
        return;
    }

    if (request.command == ipcCommandStartTun()) {
        const QString singBoxPath = request.payload.value(QStringLiteral("singBoxPath")).toString();
        const QString configPath = request.payload.value(QStringLiteral("configPath")).toString();
        const QString workingDirectory =
            request.payload.value(QStringLiteral("workingDirectory")).toString();
        const bool checkBeforeStart =
            request.payload.value(QStringLiteral("checkBeforeStart")).toBool(true);
        QString error;
        if (!m_runtime.startTun(singBoxPath, configPath, workingDirectory, checkBeforeStart,
                                &error)) {
            const bool autoDisableKillSwitch =
                request.payload.value(QStringLiteral("autoDisableKillSwitchOnFailure")).toBool(true);
            if (autoDisableKillSwitch) {
                m_killSwitch.disable(nullptr);
            }
            sendError(client, request, error);
            return;
        }
        sendOk(client, request, QJsonObject{{QStringLiteral("pid"), m_runtime.processId()}});
        return;
    }

    if (request.command == ipcCommandStopTun()) {
        QString error;
        if (!m_runtime.stopTun(&error)) {
            sendError(client, request, error);
            return;
        }
        const bool autoDisableKillSwitch =
            request.payload.value(QStringLiteral("autoDisableKillSwitch")).toBool(true);
        if (autoDisableKillSwitch) {
            m_killSwitch.disable(nullptr);
        }
        sendOk(client, request,
               QJsonObject{
                   {QStringLiteral("killSwitch"), KillSwitchManager::stateToJson(m_killSwitch.state())}});
        return;
    }

    if (request.command == ipcCommandShutdownHelper()) {
        sendOk(client, request, {});
        shutdown();
        QCoreApplication::quit();
        return;
    }

    sendError(client, request, QStringLiteral("Unknown command: %1").arg(request.command));
}

void HelperCommandServer::sendOk(QLocalSocket* client, const IpcEnvelope& request,
                                 const QJsonObject& payload)
{
    IpcEnvelope response;
    response.id = request.id;
    response.ok = true;
    response.payload = payload;
    m_server.sendResponse(client, response);
}

void HelperCommandServer::sendError(QLocalSocket* client, const IpcEnvelope& request,
                                    const QString& error)
{
    IpcEnvelope response;
    response.id = request.id;
    response.ok = false;
    response.payload = QJsonObject{{QStringLiteral("error"), error}};
    m_server.sendResponse(client, response);
    emit logLine(QStringLiteral("helper: error: %1").arg(error));
}

void HelperCommandServer::broadcastLog(const QString& line)
{
    if (!m_activeClient || line.trimmed().isEmpty()) {
        return;
    }
    IpcEnvelope event;
    event.event = ipcEventLog();
    event.payload = QJsonObject{{QStringLiteral("line"), line}};
    m_server.sendEvent(m_activeClient, event);
}

} // namespace zarya
