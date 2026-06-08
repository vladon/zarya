#pragma once

#include "cores/CoreAsset.h"
#include "domain/CoreType.h"

#include <QDateTime>
#include <QUrl>
#include <QVector>

namespace zarya {

struct CoreRelease {
    CoreType coreType = CoreType::Xray;
    QString version;
    QUrl htmlUrl;
    QVector<CoreAsset> assets;
    QString releaseNotes;
    QDateTime publishedAt;
};

} // namespace zarya
