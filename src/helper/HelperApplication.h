#pragma once

#include "helper/HelperCommandServer.h"
#include "helper/HelperPathPolicy.h"

#include <QCoreApplication>
#include <QJsonObject>

namespace zarya {

class HelperApplication : public QCoreApplication {
public:
    HelperApplication(int& argc, char** argv);

    int run();
    void setRunningAsWindowsService(bool value);
    bool prepareServiceMode(QString* errorMessage = nullptr);
    void requestServiceStop();
    int runServiceEventLoop();

private:
    bool parseArguments(QString* errorMessage);
    bool parseCommonFlags();

    int runKillSwitchCli();
    int runServiceStatusCli();
    int runServiceSelfTestCli();
    int runServiceInstallPlanCli();
    int runStopRuntimeCli();
    int runServer();

    bool ensureServiceToken(QString* errorMessage);
    QString serviceTokenPath() const;
    QJsonObject buildServiceStatusJson() const;
    QJsonObject buildServiceInstallPlanJson() const;

    bool m_devMode = false;
    bool m_serviceMode = false;
    bool m_runningAsWindowsService = false;
    bool m_recoverKillSwitchCli = false;
    bool m_killSwitchStatusCli = false;
    bool m_serviceStatusCli = false;
    bool m_serviceSelfTestCli = false;
    bool m_serviceInstallPlanCli = false;
    bool m_stopRuntimeCli = false;

    QString m_tokenFilePath;
    QString m_allowedRuntimeDir;
    QString m_allowedCoreDir;
    QString m_serverName;
    QString m_serviceName;
    QString m_ipcMode;
    QString m_logFilePath;
    QString m_allowedClientSid;

    HelperCommandServer m_commandServer;
    HelperPathPolicy m_pathPolicy;
    QString m_authToken;
};

} // namespace zarya
