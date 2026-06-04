#pragma once

#include "helper/HelperCommandServer.h"
#include "helper/HelperPathPolicy.h"

#include <QCoreApplication>

namespace zarya {

class HelperApplication : public QCoreApplication {
public:
    HelperApplication(int& argc, char** argv);

    int run();

private:
    bool parseArguments(QString* errorMessage);

    bool m_devMode = false;
    bool m_serviceMode = false;
    QString m_tokenFilePath;
    QString m_allowedRuntimeDir;
    QString m_allowedCoreDir;
    QString m_serverName;

    HelperCommandServer m_commandServer;
    HelperPathPolicy m_pathPolicy;
    QString m_authToken;
};

} // namespace zarya
