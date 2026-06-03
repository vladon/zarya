#pragma once

#include "domain/Profile.h"

#include <QString>
#include <QVector>

namespace zarya {

class ProfileStore {
public:
    explicit ProfileStore(QString filePath = {});

    QString filePath() const;
    void setFilePath(const QString& path);

    QVector<Profile> load(QString* errorMessage = nullptr) const;
    bool save(const QVector<Profile>& profiles, QString* errorMessage = nullptr) const;

private:
    QString m_filePath;
};

} // namespace zarya
