#pragma once

#include <QUrl>
#include <QString>
#include <QVector>

namespace zarya {

struct GeoDataSource {
    QString id;
    QString name;
    QString description;
    QUrl geoipUrl;
    QUrl geoipSha256Url;
    QUrl geositeUrl;
    QUrl geositeSha256Url;
    bool enabled = true;
    bool builtIn = false;
};

class GeoDataSources {
public:
    static QVector<GeoDataSource> builtInSources();
    static GeoDataSource sourceById(const QString& id);
};

} // namespace zarya
