#pragma once

#include <QUrl>
#include <QString>

namespace zarya {

struct CoreAsset {
    QString name;
    QUrl downloadUrl;
    qint64 sizeBytes = 0;
    QString sha256;
    QUrl checksumUrl;
};

} // namespace zarya
