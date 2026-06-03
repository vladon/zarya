#pragma once

#include "domain/CoreType.h"
#include "domain/Profile.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace zarya {

struct ConfigGenerationResult {
    bool success = false;
    QJsonObject config;
    QString errorMessage;
};

class ICoreAdapter {
public:
    virtual ~ICoreAdapter() = default;

    virtual CoreType type() const = 0;
    virtual QString displayName() const = 0;
    virtual ConfigGenerationResult generateConfig(const Profile& profile) const = 0;
    virtual QStringList argumentsForConfig(const QString& configPath) const = 0;
};

} // namespace zarya
