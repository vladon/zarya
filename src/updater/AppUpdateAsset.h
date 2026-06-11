#pragma once

#include <QJsonObject>
#include <QString>

namespace zarya {

struct AppUpdateSignature {
    QString type;
    QString url;
};

struct AppUpdateAsset {
    QString platform;
    QString architecture;
    QString installationMode;
    QString fileName;
    QString url;
    qint64 sizeBytes = 0;
    QString sha256;
    AppUpdateSignature signature;

    bool isValid() const;
    static AppUpdateAsset fromJson(const QJsonObject& object);
    QJsonObject toJson() const;
};

} // namespace zarya
