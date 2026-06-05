#pragma once

#include <QString>

namespace zarya {

class AppPaths {
public:
    static void initialize(bool portableRequested);

    static bool isPortableMode();
    static QString applicationDir();
    static QString dataDir();
    static QString configDir();
    static QString configFilePath();
    static QString appDataDir();
    static QString coresDir();
    static QString profilesFilePath();
    static QString subscriptionsFilePath();
    static QString routingFilePath();
    static QString dnsFilePath();
    static QString runtimeDir();
    static QString xrayConfigPath();
    static QString singBoxConfigPath();
    static QString singBoxTunConfigPath();
    static QString helperTokenPath();
    static QString killSwitchDir();
    static QString killSwitchMarkerPath();
    static QString killSwitchRulesFilePath();
    static void ensureKillSwitchDir();
    static QString resolvedHelperPath();
    static QString singBoxRuleSetDir();
    static QString singBoxRuleSetSourceDir();
    static QString localRuleSetSrsPath(const QString& tag);
    static QString localRuleSetJsonPath(const QString& tag);
    static QString ruleSetStorePath();
    static QString singBoxCoreDir();
    static QString testRuntimeDir();
    static QString testConfigPath(const QString& profileId);
    static QString portableFlagPath();
    static QString geoDataDir();
    static QString xrayResourceDir();
    static QString xrayCoreDir();

private:
    static void ensureDir(const QString& path);
    static bool s_portableMode;
};

} // namespace zarya
