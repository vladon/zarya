#include "dns/DnsManager.h"

#include "storage/AppSettings.h"

#include <QFile>
#include <QUuid>

namespace zarya {

DnsManager::DnsManager(DnsStore store)
    : m_store(std::move(store))
{
}

void DnsManager::load()
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

bool DnsManager::save(QString* errorMessage)
{
    return m_store.save(m_profiles, errorMessage);
}

QVector<DnsProfile> DnsManager::profiles() const
{
    return m_profiles;
}

QString DnsManager::activeProfileId() const
{
    return m_activeProfileId;
}

DnsProfile DnsManager::activeProfile() const
{
    return profileById(m_activeProfileId);
}

DnsProfile DnsManager::profileById(const QString& id) const
{
    for (const DnsProfile& profile : m_profiles) {
        if (profile.id == id) {
            return profile;
        }
    }
    return DnsProfile::builtInSystemDns();
}

bool DnsManager::setActiveProfileId(const QString& profileId)
{
    if (profileId.isEmpty() || profileById(profileId).id != profileId) {
        return false;
    }
    m_activeProfileId = profileId;
    AppSettings::instance().setSelectedDnsProfileId(profileId);
    return true;
}

bool DnsManager::upsertProfile(const DnsProfile& profile)
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
    DnsProfile copy = profile;
    if (copy.id.isEmpty()) {
        copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    copy.isBuiltIn = false;
    copy.mode = DnsProfileMode::Custom;
    copy.createdAt = QDateTime::currentDateTimeUtc();
    copy.updatedAt = copy.createdAt;
    m_profiles.append(copy);
    return true;
}

bool DnsManager::removeProfile(const QString& id, QString* errorMessage)
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            if (m_profiles[i].isBuiltIn) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Built-in DNS profiles cannot be deleted.");
                }
                return false;
            }
            m_profiles.removeAt(i);
            if (m_activeProfileId == id) {
                setActiveProfileId(DnsBuiltinIds::systemDns());
            }
            return true;
        }
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("DNS profile not found.");
    }
    return false;
}

DnsProfile DnsManager::duplicateProfile(const QString& id, QString* errorMessage)
{
    const DnsProfile source = profileById(id);
    if (source.id != id) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("DNS profile not found.");
        }
        return {};
    }

    DnsProfile copy = source;
    copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.name = QStringLiteral("%1 (copy)").arg(source.name);
    copy.isBuiltIn = false;
    copy.mode = DnsProfileMode::Custom;
    copy.createdAt = QDateTime::currentDateTimeUtc();
    copy.updatedAt = copy.createdAt;
    m_profiles.append(copy);
    return copy;
}

QString DnsManager::filePath() const
{
    return m_store.filePath();
}

void DnsManager::ensureBuiltIns()
{
    const QVector<DnsProfile> builtIns = DnsProfile::createBuiltInProfiles();
    for (const DnsProfile& builtIn : builtIns) {
        bool found = false;
        for (DnsProfile& profile : m_profiles) {
            if (profile.id == builtIn.id) {
                profile.isBuiltIn = true;
                profile.mode = builtIn.mode;
                if (profile.name.trimmed().isEmpty()) {
                    profile.name = builtIn.name;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            m_profiles.append(builtIn);
        }
    }
}

void DnsManager::resolveActiveProfile()
{
    QString selected = AppSettings::instance().selectedDnsProfileId();
    if (selected.isEmpty() || profileById(selected).id != selected) {
        selected = DnsBuiltinIds::systemDns();
    }
    m_activeProfileId = selected;
    AppSettings::instance().setSelectedDnsProfileId(m_activeProfileId);
}

} // namespace zarya
