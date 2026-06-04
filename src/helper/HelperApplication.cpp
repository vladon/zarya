#include "helper/HelperApplication.h"

#include "helper/HelperAuth.h"
#include "ipc/IpcTransport.h"
#include "storage/AppPaths.h"

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

int HelperApplication::run()
{
    QString error;
    if (!parseArguments(&error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }

    if (!m_commandServer.start(m_serverName, m_authToken, m_pathPolicy, &error)) {
        fprintf(stderr, "zarya-helper: %s\n", error.toUtf8().constData());
        return 1;
    }

    return exec();
}

} // namespace zarya
