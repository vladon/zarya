#include "routing/RoutingManager.h"

#include "routing/RoutingGeoUtils.h"
#include "storage/AppSettings.h"

#include <QFile>
#include <QUuid>

namespace zarya {

RoutingManager::RoutingManager(RoutingStore store)
    : m_store(std::move(store))
{
}

void RoutingManager::load()
{
    const bool fileExisted = QFile::exists(m_store.filePath());
    QString error;
    m_profiles = m_store.load(&error);
    ensureBuiltIns();
    resolveActiveProfile();
    if (!fileExisted) {
        save();
    }
}

bool RoutingManager::save(QString* errorMessage)
{
    return m_store.save(m_profiles, errorMessage);
}

QVector<RoutingProfile> RoutingManager::profiles() const
{
    return m_profiles;
}

QString RoutingManager::activeProfileId() const
{
    return m_activeProfileId;
}

RoutingProfile RoutingManager::activeProfile() const
{
    return profileById(m_activeProfileId);
}

RoutingProfile RoutingManager::profileById(const QString& id) const
{
    for (const RoutingProfile& profile : m_profiles) {
        if (profile.id == id) {
            return profile;
        }
    }
    return RoutingProfile::builtInProxyAll();
}

bool RoutingManager::setActiveProfileId(const QString& profileId)
{
    if (profileId.isEmpty() || profileById(profileId).id != profileId) {
        return false;
    }
    m_activeProfileId = profileId;
    AppSettings::instance().setSelectedRoutingProfileId(profileId);
    return true;
}

bool RoutingManager::upsertProfile(const RoutingProfile& profile)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == profile.id) {
            if (m_profiles[i].isBuiltIn) {
                return false;
            }
            m_profiles[i] = profile;
            m_profiles[i].updatedAt = QDateTime::currentDateTimeUtc();
            return true;
        }
    }
    RoutingProfile copy = profile;
    if (copy.id.isEmpty()) {
        copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    copy.isBuiltIn = false;
    copy.createdAt = QDateTime::currentDateTimeUtc();
    copy.updatedAt = copy.createdAt;
    m_profiles.append(copy);
    return true;
}

bool RoutingManager::removeProfile(const QString& id, QString* errorMessage)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id != id) {
            continue;
        }
        if (m_profiles[i].isBuiltIn) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Built-in routing profiles cannot be deleted.");
            }
            return false;
        }
        m_profiles.removeAt(i);
        if (m_activeProfileId == id) {
            setActiveProfileId(RoutingBuiltinIds::bypassLan());
        }
        return true;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Routing profile not found.");
    }
    return false;
}

RoutingProfile RoutingManager::duplicateProfile(const QString& id, QString* errorMessage)
{
    const RoutingProfile source = profileById(id);
    if (source.id != id) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Routing profile not found.");
        }
        return {};
    }

    RoutingProfile copy = source;
    copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.name = QStringLiteral("%1 (copy)").arg(source.name);
    copy.isBuiltIn = false;
    copy.mode = RoutingMode::Custom;
    copy.createdAt = QDateTime::currentDateTimeUtc();
    copy.updatedAt = copy.createdAt;
    for (RoutingRule& rule : copy.rules) {
        rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    m_profiles.append(copy);
    return copy;
}

QString RoutingManager::filePath() const
{
    return m_store.filePath();
}

RoutingProfile RoutingManager::proxyAllProfile()
{
    return RoutingProfile::builtInProxyAll();
}

void RoutingManager::ensureBuiltIns()
{
    const QVector<RoutingProfile> builtIns = RoutingProfile::createBuiltInProfiles();
    for (const RoutingProfile& builtIn : builtIns) {
        bool found = false;
        for (const RoutingProfile& existing : m_profiles) {
            if (existing.id == builtIn.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_profiles.append(builtIn);
        }
    }
}

void RoutingManager::resolveActiveProfile()
{
    QString selected = AppSettings::instance().selectedRoutingProfileId();
    if (selected.isEmpty() || profileById(selected).id != selected) {
        selected = RoutingBuiltinIds::bypassLan();
        if (profileById(selected).id != selected) {
            selected = RoutingBuiltinIds::proxyAll();
        }
    }
    m_activeProfileId = selected;
    AppSettings::instance().setSelectedRoutingProfileId(m_activeProfileId);
}

bool RoutingManager::activeProfileUsesGeoData() const
{
    return profileUsesGeoData(activeProfile());
}

bool RoutingManager::profileUsesGeoData(const RoutingProfile& profile)
{
    return RoutingGeoUtils::profileUsesGeoData(profile);
}

QStringList RoutingManager::geoTagsUsed(const RoutingProfile& profile)
{
    return RoutingGeoUtils::geoTagsUsed(profile);
}

} // namespace zarya
