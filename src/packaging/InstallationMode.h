#pragma once

#include <QJsonObject>
#include <QString>

namespace zarya {

enum class InstallationMode {
    Portable,
    Installed,
    Unknown,
};

class InstallationInfo {
public:
    static InstallationMode detect();
    static QString modeString(InstallationMode mode);
    static QString currentModeString();
    static bool portableFlagPresent();
    static bool installedMarkerPresent();
    static bool appDirWritable();
    static QJsonObject diagnosticsJson();
};

} // namespace zarya
