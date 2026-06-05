#pragma once

#include <QString>

namespace zarya {

struct KillSwitchMarkerData {
    int version = 1;
    QString backend;
    QString enabledAt;
    QString rulesetName;
    QString tunInterfaceName;
};

class KillSwitchMarker {
public:
    static QString markerPath();
    static bool exists();
    static bool read(KillSwitchMarkerData* data, QString* errorMessage = nullptr);
    static bool write(const KillSwitchMarkerData& data, QString* errorMessage = nullptr);
    static bool remove(QString* errorMessage = nullptr);
};

} // namespace zarya
