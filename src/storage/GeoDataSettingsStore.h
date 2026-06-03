#pragma once

#include <QString>

namespace zarya {

class GeoDataSettingsStore {
public:
    static GeoDataSettingsStore& instance();

    QString selectedSourceId() const;
    void setSelectedSourceId(const QString& sourceId);

    bool autoCheckOnStartup() const;
    void setAutoCheckOnStartup(bool enabled);

    bool warnIfMissing() const;
    void setWarnIfMissing(bool enabled);

private:
    GeoDataSettingsStore() = default;
};

} // namespace zarya
