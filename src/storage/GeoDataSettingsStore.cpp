#include "storage/GeoDataSettingsStore.h"

#include "storage/AppSettings.h"

#include <QSettings>

namespace zarya {

GeoDataSettingsStore& GeoDataSettingsStore::instance()
{
    static GeoDataSettingsStore store;
    return store;
}

QString GeoDataSettingsStore::selectedSourceId() const
{
    const QString value =
        AppSettings::settings().value(QStringLiteral("geodata/selectedSourceId")).toString();
    return value.isEmpty() ? QStringLiteral("loyalsoldier") : value;
}

void GeoDataSettingsStore::setSelectedSourceId(const QString& sourceId)
{
    AppSettings::settings().setValue(QStringLiteral("geodata/selectedSourceId"),
                                       sourceId.trimmed());
}

bool GeoDataSettingsStore::autoCheckOnStartup() const
{
    return AppSettings::settings()
        .value(QStringLiteral("geodata/autoCheckOnStartup"), true)
        .toBool();
}

void GeoDataSettingsStore::setAutoCheckOnStartup(bool enabled)
{
    AppSettings::settings().setValue(QStringLiteral("geodata/autoCheckOnStartup"), enabled);
}

bool GeoDataSettingsStore::warnIfMissing() const
{
    return AppSettings::settings().value(QStringLiteral("geodata/warnIfMissing"), true).toBool();
}

void GeoDataSettingsStore::setWarnIfMissing(bool enabled)
{
    AppSettings::settings().setValue(QStringLiteral("geodata/warnIfMissing"), enabled);
}

} // namespace zarya
