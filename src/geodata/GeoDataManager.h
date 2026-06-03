#pragma once

#include "geodata/GeoDataFileStatus.h"
#include "geodata/GeoDataSource.h"

#include <QObject>
#include <QVector>
#include <atomic>

class QThread;

namespace zarya {

class GeoDataManager : public QObject {
    Q_OBJECT

public:
    explicit GeoDataManager(QObject* parent = nullptr);
    ~GeoDataManager() override;

    GeoDataFileStatus checkFileStatus(GeoDataKind kind) const;
    QVector<GeoDataFileStatus> checkAllStatus() const;

    void updateGeoIp();
    void updateGeoSite();
    void updateAll();
    void verifyAll();

    bool cancel();

    QString targetDirectory() const;
    bool isTargetWritable(QString* errorMessage = nullptr) const;

    bool profileNeedsGeoIp(const QStringList& geoTags) const;
    bool profileNeedsGeoSite(const QStringList& geoTags) const;
    bool hasRequiredFilesForTags(const QStringList& geoTags) const;
    QStringList missingFileNamesForTags(const QStringList& geoTags) const;

signals:
    void statusChanged(QVector<GeoDataFileStatus> statuses);
    void progressChanged(GeoDataKind kind, qint64 received, qint64 total);
    void updateFinished(bool ok);
    void logLine(QString line);

private:
    void startUpdate(const QVector<GeoDataKind>& kinds);
    void runUpdate(const QVector<GeoDataKind>& kinds);
    bool updateFile(GeoDataKind kind, GeoDataSource source, const QString& userAgent);
    void setVerifiedHash(GeoDataKind kind, const QString& hash) const;
    QString verifiedHash(GeoDataKind kind) const;
    QString settingsKeyForVerifiedHash(GeoDataKind kind) const;

    std::atomic<bool> m_cancelRequested{false};
    QThread* m_updateThread = nullptr;
};

} // namespace zarya
