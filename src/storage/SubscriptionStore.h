#pragma once

#include "domain/Subscription.h"

#include <QString>
#include <QVector>

namespace zarya {

class SubscriptionStore {
public:
    explicit SubscriptionStore(QString filePath = {});

    QString filePath() const;
    void setFilePath(const QString& path);

    QVector<Subscription> load(QString* errorMessage = nullptr) const;
    bool save(const QVector<Subscription>& subscriptions, QString* errorMessage = nullptr) const;

private:
    QString m_filePath;
};

} // namespace zarya
