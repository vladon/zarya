#pragma once

#include "domain/RoutingProfile.h"
#include "storage/RoutingStore.h"

#include <QString>

namespace zarya {

class RoutingManager {
public:
    explicit RoutingManager(RoutingStore store = RoutingStore());

    void load();
    bool save(QString* errorMessage = nullptr);

    QVector<RoutingProfile> profiles() const;
    RoutingProfile activeProfile() const;
    QString activeProfileId() const;
    bool setActiveProfileId(const QString& profileId);

    RoutingProfile profileById(const QString& id) const;
    bool upsertProfile(const RoutingProfile& profile);
    bool removeProfile(const QString& id, QString* errorMessage = nullptr);
    RoutingProfile duplicateProfile(const QString& id, QString* errorMessage = nullptr);

    QString filePath() const;

    static RoutingProfile proxyAllProfile();

    bool activeProfileUsesGeoData() const;
    static bool profileUsesGeoData(const RoutingProfile& profile);
    static QStringList geoTagsUsed(const RoutingProfile& profile);

private:
    void ensureBuiltIns();
    void resolveActiveProfile();

    RoutingStore m_store;
    QVector<RoutingProfile> m_profiles;
    QString m_activeProfileId;
};

} // namespace zarya
