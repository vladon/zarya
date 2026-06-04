#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct TunSupportResult {
    bool supported = false;
    bool hasRequiredPrivileges = false;
    QString platform;
    QString reason;
    QStringList warnings;
};

class SingBoxTunSupportChecker {
public:
    static TunSupportResult check(const QString& singBoxExecutablePath);
};

} // namespace zarya
