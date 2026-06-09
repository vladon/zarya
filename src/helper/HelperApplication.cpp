#include "helper/HelperApplication.h"

#include "app/BuildInfo.h"
#include "helper/HelperAuth.h"
#include "ipc/IpcTransport.h"
#include "killswitch/KillSwitchManager.h"
#include "platform/PlatformPrivilege.h"
#include "storage/AppPaths.h"

#include <QJsonDocument>
#include <cstdio>

namespace zarya {

HelperApplication::HelperApplication(int& argc, char** argv)
    : QCoreApplication(argc, argv)
{
    setApplicationName(QStringLiteral("zarya-helper"));
}

bool HelperApplication::parseArguments(QString* errorMessage)
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
        } else if (arg == QStringLiteral("--token-file") && index + 1 < args.size()) {
            m_tokenFilePath = args.at(++index);
        } else if (arg == QStringLiteral("--allowed-runtime-dir") && index + 1 < args.size()) {
            m_allowedRuntimeDir = args.at(++index);
        } else if (arg == QStringLiteral("--allowed-core-dir") && index + 1 < args.size()) {
            m_allowedCoreDir = args.at(++index);
        } else if (arg == QStringLiteral("--server-name") && index + 1 < args.size()) {
            m_serverName = args.at(++index);
        }
    }

    if (m_recoverKillSwitchCli || m_killSwitchStatusCli) {
        AppPaths::initialize(false);
        return true;
    }

    if (m_tokenFilePath.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("--token-file is required.");
        }
        return false;
    }

    if (!HelperAuth::loadTokenFromFile(m_tokenFilePath, &m_authToken, errorMessage)) {
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

    if (m_serverName.isEmpty()) {
        m_serverName = IpcTransport::defaultServerName();
    }

    Q_UNUSED(m_devMode);
    Q_UNUSED(m_serviceMode);
    return true;
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
                    "  --token-file <path>       Auth token file (required for server mode)\n"
                    "  --allowed-runtime-dir <p>   Allowed runtime directory\n"
                    "  --allowed-core-dir <p>    Allowed sing-box core directory\n"
                    "  --server-name <name>      IPC server name\n"
                    "  --dev                     Development mode\n"
                    "  --service                 Service mode\n"
                    "  --recover-killswitch      Recover kill switch state\n"
                    "  --killswitch-status       Print kill switch status JSON\n",
                    BuildInfo::appVersion().toUtf8().constData());
            return 0;
        }
    }

    QString error;
    if (!parseArguments(&error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }

    if (m_recoverKillSwitchCli || m_killSwitchStatusCli) {
        return runKillSwitchCli();
    }

    if (!m_commandServer.start(m_serverName, m_authToken, m_pathPolicy, &error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }

    return exec();
}

} // namespace zarya
