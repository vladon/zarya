#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

enum class HelperServiceInstallState {
    NotInstalled,
    Installed,
    Running,
    Stopped,
    Failed,
    Unsupported,
    Unknown,
    DesignOnly,
};

QString helperServiceInstallStateToString(HelperServiceInstallState state);

struct HelperServiceStatus {
    HelperServiceInstallState state = HelperServiceInstallState::Unknown;
    QString backend;
    QString serviceName;
    QString version;
    bool privileged = false;
    bool connected = false;
    QString executablePath;
    QString lastError;
    QStringList warnings;
};

} // namespace zarya
