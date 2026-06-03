#pragma once

#include "domain/DnsProfile.h"

#include <QString>
#include <QVector>

namespace zarya {

class DnsStore {
public:
    explicit DnsStore(QString filePath = {});

    QString filePath() const;
    void setFilePath(const QString& path);

    QVector<DnsProfile> load(QString* errorMessage = nullptr) const;
    bool save(const QVector<DnsProfile>& profiles, QString* errorMessage = nullptr) const;

private:
    QString m_filePath;
};

} // namespace zarya
