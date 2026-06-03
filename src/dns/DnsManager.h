#pragma once

#include "domain/DnsProfile.h"
#include "storage/DnsStore.h"

#include <QString>

namespace zarya {

class DnsManager {
public:
    explicit DnsManager(DnsStore store = DnsStore());

    void load();
    bool save(QString* errorMessage = nullptr);

    QVector<DnsProfile> profiles() const;
    DnsProfile activeProfile() const;
    QString activeProfileId() const;
    bool setActiveProfileId(const QString& profileId);

    DnsProfile profileById(const QString& id) const;
    bool upsertProfile(const DnsProfile& profile);
    bool removeProfile(const QString& id, QString* errorMessage = nullptr);
    DnsProfile duplicateProfile(const QString& id, QString* errorMessage = nullptr);

    QString filePath() const;

private:
    void ensureBuiltIns();
    void resolveActiveProfile();

    DnsStore m_store;
    QVector<DnsProfile> m_profiles;
    QString m_activeProfileId;
};

} // namespace zarya
