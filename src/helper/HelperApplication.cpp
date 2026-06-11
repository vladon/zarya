#include "helper/HelperApplication.h"

#include "app/BuildInfo.h"
#include "helper/HelperAuth.h"
#include "ipc/IpcTransport.h"
#include "killswitch/KillSwitchManager.h"
#include "platform/PlatformPrivilege.h"
#include "service/HelperServiceIdentity.h"
#include "service/HelperServiceCommandLine.h"
#include "storage/AppPaths.h"
#include "storage/HelperSession.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimer>
#include <cstdio>

namespace zarya {

HelperApplication::HelperApplication(int& argc, char** argv)
    : QCoreApplication(argc, argv)
{
    setApplicationName(QStringLiteral("zarya-helper"));
}

void HelperApplication::setRunningAsWindowsService(bool value)
{
    m_runningAsWindowsService = value;
    if (value) {
        m_serviceMode = true;
    }
}

QString HelperApplication::serviceTokenPath() const
{
    if (!m_allowedRuntimeDir.isEmpty()) {
        return QDir(m_allowedRuntimeDir).filePath(QStringLiteral("helper-service.token"));
    }
    AppPaths::initialize(false);
    return AppPaths::runtimeDir() + QStringLiteral("/helper-service.token");
}

bool HelperApplication::ensureServiceToken(QString* errorMessage)
{
    const QString path = serviceTokenPath();
    QFile existing(path);
    if (existing.exists() && existing.open(QIODevice::ReadOnly)) {
        const QString token = QString::fromUtf8(existing.readAll()).trimmed();
        if (!token.isEmpty()) {
            m_authToken = token;
            return true;
        }
    }

    const QString token = HelperSession::ensureSessionToken(errorMessage);
    if (token.isEmpty()) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(token.toUtf8());
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    m_authToken = token;
    return true;
}

bool HelperApplication::parseCommonFlags()
{
    const QStringList args = arguments();
    for (int index = 1; index < args.size(); ++index) {
        const QString& arg = args.at(index);
        if (arg == QStringLiteral("--dev")) {
            m_devMode = true;
        } else if (arg == QStringLiteral("--service")) {
            m_serviceMode = true;
        } else if (arg == QStringLiteral("--recover-killswitch")) {
            m_recoverKillSwitchCli = true;
        } else if (arg == QStringLiteral("--killswitch-status")) {
            m_killSwitchStatusCli = true;
        } else if (arg == QStringLiteral("--service-status")) {
            m_serviceStatusCli = true;
        } else if (arg == QStringLiteral("--service-self-test")) {
            m_serviceSelfTestCli = true;
        } else if (arg == QStringLiteral("--service-install-plan")) {
            m_serviceInstallPlanCli = true;
        } else if (arg == QStringLiteral("--stop-runtime")) {
            m_stopRuntimeCli = true;
        } else if (arg == QStringLiteral("--token-file") && index + 1 < args.size()) {
            m_tokenFilePath = args.at(++index);
        } else if (arg == QStringLiteral("--allowed-runtime-dir") && index + 1 < args.size()) {
            m_allowedRuntimeDir = args.at(++index);
        } else if (arg == QStringLiteral("--allowed-core-dir") && index + 1 < args.size()) {
            m_allowedCoreDir = args.at(++index);
        } else if (arg == QStringLiteral("--server-name") && index + 1 < args.size()) {
            m_serverName = args.at(++index);
        } else if (arg == QStringLiteral("--service-name") && index + 1 < args.size()) {
            m_serviceName = args.at(++index);
        } else if (arg == QStringLiteral("--ipc-mode") && index + 1 < args.size()) {
            m_ipcMode = args.at(++index);
        } else if (arg == QStringLiteral("--log-file") && index + 1 < args.size()) {
            m_logFilePath = args.at(++index);
        } else if (arg == QStringLiteral("--allowed-client-sid") && index + 1 < args.size()) {
            m_allowedClientSid = args.at(++index);
        }
    }
    return true;
}

bool HelperApplication::parseArguments(QString* errorMessage)
{
    parseCommonFlags();

    if (m_recoverKillSwitchCli || m_killSwitchStatusCli || m_serviceStatusCli
        || m_serviceSelfTestCli || m_serviceInstallPlanCli || m_stopRuntimeCli) {
        AppPaths::initialize(false);
        return true;
    }

    if (m_serviceMode) {
        if (m_allowedRuntimeDir.isEmpty()) {
            AppPaths::initialize(false);
            m_allowedRuntimeDir = AppPaths::runtimeDir();
        }
        if (m_allowedCoreDir.isEmpty()) {
            AppPaths::initialize(false);
            m_allowedCoreDir = AppPaths::singBoxCoreDir();
        }
        if (m_serviceName.isEmpty()) {
            m_serviceName = HelperServiceIdentity::internalServiceName();
        }
        if (m_serverName.isEmpty()) {
            m_serverName = IpcTransport::serviceServerName(m_serviceName);
        }
        if (m_tokenFilePath.isEmpty()) {
            if (!ensureServiceToken(errorMessage)) {
                return false;
            }
        } else if (!HelperAuth::loadTokenFromFile(m_tokenFilePath, &m_authToken, errorMessage)) {
            return false;
        }
    } else if (m_tokenFilePath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("--token-file is required.");
        }
        return false;
    } else if (!HelperAuth::loadTokenFromFile(m_tokenFilePath, &m_authToken, errorMessage)) {
        return false;
    }

    if (m_allowedRuntimeDir.isEmpty()) {
        AppPaths::initialize(false);
        m_allowedRuntimeDir = AppPaths::runtimeDir();
    }
    if (m_allowedCoreDir.isEmpty()) {
        AppPaths::initialize(false);
        m_allowedCoreDir = AppPaths::singBoxCoreDir();
    }

    m_pathPolicy.setAllowedRuntimeDir(m_allowedRuntimeDir);
    m_pathPolicy.setAllowedCoreDir(m_allowedCoreDir);
    if (!m_allowedClientSid.isEmpty()) {
        m_pathPolicy.setAllowedClientSid(m_allowedClientSid);
    }

    if (m_serverName.isEmpty()) {
        m_serverName = m_serviceMode ? IpcTransport::serviceServerName(m_serviceName)
                                     : IpcTransport::defaultServerName();
    }

    Q_UNUSED(m_devMode);
    Q_UNUSED(m_ipcMode);
    Q_UNUSED(m_logFilePath);
    return true;
}

bool HelperApplication::prepareServiceMode(QString* errorMessage)
{
    m_serviceMode = true;
    return parseArguments(errorMessage);
}

void HelperApplication::requestServiceStop()
{
    m_commandServer.shutdown();
    quit();
}

int HelperApplication::runServiceEventLoop()
{
    QString error;
    if (!runServer()) {
        return 1;
    }
    return exec();
}

QJsonObject HelperApplication::buildServiceStatusJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), BuildInfo::appVersion());
    object.insert(QStringLiteral("serviceName"),
                  m_serviceName.isEmpty() ? HelperServiceIdentity::internalServiceName()
                                          : m_serviceName);
    object.insert(QStringLiteral("serviceMode"), m_serviceMode);
    object.insert(QStringLiteral("windowsService"), m_runningAsWindowsService);
    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    object.insert(QStringLiteral("privileged"), privileges.elevated);
    object.insert(QStringLiteral("allowedRuntimeDir"), m_allowedRuntimeDir);
    object.insert(QStringLiteral("allowedCoreDir"), m_allowedCoreDir);
    object.insert(QStringLiteral("serverName"), m_serverName);
    return object;
}

QJsonObject HelperApplication::buildServiceInstallPlanJson() const
{
    HelperServiceInstallOptions options;
    options.helperExecutablePath = QCoreApplication::applicationFilePath();
    options.serviceName = HelperServiceIdentity::internalServiceName();
    options.displayName = HelperServiceIdentity::displayName();
    options.allowedRuntimeDir = m_allowedRuntimeDir.isEmpty() ? AppPaths::runtimeDir()
                                                              : m_allowedRuntimeDir;
    options.allowedCoreDir = m_allowedCoreDir.isEmpty() ? AppPaths::singBoxCoreDir()
                                                        : m_allowedCoreDir;
    options.logFilePath = options.allowedRuntimeDir + QStringLiteral("/helper-service.log");
    options.manualStart = true;

    QJsonObject object;
    object.insert(QStringLiteral("serviceName"), options.serviceName);
    object.insert(QStringLiteral("displayName"), options.displayName);
    object.insert(QStringLiteral("binaryPath"), HelperServiceCommandLine::buildBinaryPath(options));
#if defined(Q_OS_WIN)
    object.insert(QStringLiteral("manualInstall"),
                  QStringLiteral("sc.exe create %1 binPath= \"%2\" start= demand DisplayName= \"%3\"")
                      .arg(options.serviceName,
                           HelperServiceCommandLine::buildBinaryPath(options),
                           options.displayName));
#else
    object.insert(QStringLiteral("manualInstall"),
                  QStringLiteral("See docs/service/ for platform-specific install commands."));
#endif
    return object;
}

int HelperApplication::runServiceStatusCli()
{
    fprintf(stdout, "%s\n",
            QJsonDocument(buildServiceStatusJson()).toJson(QJsonDocument::Indented).constData());
    return 0;
}

int HelperApplication::runServiceSelfTestCli()
{
    QJsonObject object = buildServiceStatusJson();
    QStringList failures;

    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    object.insert(QStringLiteral("privilegeSummary"), privileges.summary);

    if (m_allowedRuntimeDir.isEmpty() || !QDir(m_allowedRuntimeDir).exists()) {
        failures.append(QStringLiteral("allowed-runtime-dir missing or invalid"));
    }
    if (m_allowedCoreDir.isEmpty() || !QDir(m_allowedCoreDir).exists()) {
        failures.append(QStringLiteral("allowed-core-dir missing or invalid"));
    }

    KillSwitchManager manager;
    manager.refreshStartupState(privileges.elevated);
    object.insert(QStringLiteral("killSwitchSupported"), manager.state().supported);
    object.insert(QStringLiteral("killSwitchBackend"), manager.state().backend);

    object.insert(QStringLiteral("ok"), failures.isEmpty());
    if (!failures.isEmpty()) {
        QJsonArray array;
        for (const QString& failure : failures) {
            array.append(failure);
        }
        object.insert(QStringLiteral("failures"), array);
    }

    fprintf(stdout, "%s\n", QJsonDocument(object).toJson(QJsonDocument::Indented).constData());
    return failures.isEmpty() ? 0 : 1;
}

int HelperApplication::runServiceInstallPlanCli()
{
    AppPaths::initialize(false);
    fprintf(stdout, "%s\n",
            QJsonDocument(buildServiceInstallPlanJson()).toJson(QJsonDocument::Indented).constData());
    return 0;
}

int HelperApplication::runStopRuntimeCli()
{
    QString error;
    if (!m_commandServer.start(m_serverName, m_authToken, m_pathPolicy, &error)) {
        m_commandServer.shutdown();
    }
    fprintf(stdout, "zarya-helper: stop-runtime is only meaningful for a running helper instance\n");
    return 0;
}

int HelperApplication::runKillSwitchCli()
{
    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    KillSwitchManager manager;
    manager.refreshStartupState(privileges.elevated);

    if (m_killSwitchStatusCli) {
        const QJsonObject object = KillSwitchManager::stateToJson(manager.state());
        fprintf(stdout, "%s\n", QJsonDocument(object).toJson(QJsonDocument::Indented).constData());
        return 0;
    }

    QString error;
    if (!manager.recover(true, &error)) {
        fprintf(stderr, "zarya-helper: kill switch recovery failed: %s\n", error.toUtf8().constData());
        return 1;
    }
    fprintf(stdout, "zarya-helper: kill switch recovered\n");
    return 0;
}

int HelperApplication::runServer()
{
    QString error;
    if (!m_commandServer.start(m_serverName, m_authToken, m_pathPolicy, &error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }
    return 0;
}

int HelperApplication::run()
{
    const QStringList args = arguments();
    for (int index = 1; index < args.size(); ++index) {
        const QString& arg = args.at(index);
        if (arg == QStringLiteral("--version") || arg == QStringLiteral("-v")) {
            fprintf(stdout, "%s\n", BuildInfo::helperCliVersionText().toUtf8().constData());
            return 0;
        }
        if (arg == QStringLiteral("--help") || arg == QStringLiteral("-h")) {
            fprintf(stdout,
                    "zarya-helper %s\n"
                    "Usage: zarya-helper --token-file <path> [options]\n"
                    "Options:\n"
                    "  --version                 Show version\n"
                    "  --help                    Show this help\n"
                    "  --token-file <path>       Auth token file (dev/manual mode)\n"
                    "  --allowed-runtime-dir <p> Allowed runtime directory\n"
                    "  --allowed-core-dir <p>    Allowed sing-box core directory\n"
                    "  --server-name <name>      IPC server name\n"
                    "  --dev                     Development mode\n"
                    "  --service                 Service mode\n"
                    "  --service-name <name>     Service identity (default ZaryaHelper)\n"
                    "  --ipc-mode <mode>         IPC mode (local-socket)\n"
                    "  --log-file <path>         Service log file path\n"
                    "  --allowed-client-sid <sid> Allowed Windows client SID\n"
                    "  --service-status          Print service status JSON\n"
                    "  --service-self-test       Run helper self-test\n"
                    "  --service-install-plan    Print install plan JSON\n"
                    "  --recover-killswitch      Recover kill switch state\n"
                    "  --killswitch-status       Print kill switch status JSON\n"
                    "  --stop-runtime            Request runtime stop (running instance)\n",
                    BuildInfo::appVersion().toUtf8().constData());
            return 0;
        }
    }

    QString error;
    if (!parseArguments(&error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }

    if (m_serviceStatusCli) {
        return runServiceStatusCli();
    }
    if (m_serviceSelfTestCli) {
        return runServiceSelfTestCli();
    }
    if (m_serviceInstallPlanCli) {
        return runServiceInstallPlanCli();
    }
    if (m_stopRuntimeCli) {
        return runStopRuntimeCli();
    }
    if (m_recoverKillSwitchCli || m_killSwitchStatusCli) {
        return runKillSwitchCli();
    }

    if (runServer() != 0) {
        return 1;
    }
    return exec();
}

} // namespace zarya
