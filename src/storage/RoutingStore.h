#pragma once

#include "domain/RoutingProfile.h"

#include <QString>
#include <QVector>

namespace zarya {

class RoutingStore {
public:
    explicit RoutingStore(QString filePath = {});

    QString filePath() const;
    void setFilePath(const QString& path);

    QVector<RoutingProfile> load(QString* errorMessage = nullptr) const;
    bool save(const QVector<RoutingProfile>& profiles, QString* errorMessage = nullptr) const;

private:
    QString m_filePath;
};

} // namespace zarya
