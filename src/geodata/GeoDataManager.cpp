#include "geodata/GeoDataManager.h"

#include "geodata/GeoDataDownloader.h"
#include "geodata/GeoDataSource.h"
#include "geodata/GeoDataVerifier.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "storage/GeoDataSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QSettings>
#include <QThread>
#include <QUrl>

namespace zarya {

namespace {

QString userAgentString()
{
#ifndef ZARYA_VERSION_STRING
#define ZARYA_VERSION_STRING "0.11.0"
#endif
    return QStringLiteral("Zarya/%1").arg(QStringLiteral(ZARYA_VERSION_STRING));
}

QUrl urlForKind(const GeoDataSource& source, GeoDataKind kind, bool checksum)
{
    switch (kind) {
    case GeoDataKind::GeoSite:
        return checksum ? source.geositeSha256Url : source.geositeUrl;
    case GeoDataKind::GeoIp:
        return checksum ? source.geoipSha256Url : source.geoipUrl;
    }
    return {};
}

} // namespace

GeoDataManager::GeoDataManager(QObject* parent)
    : QObject(parent)
{
}

GeoDataManager::~GeoDataManager()
{
    cancel();
    if (m_updateThread) {
        m_updateThread->wait();
        delete m_updateThread;
        m_updateThread = nullptr;
    }
}

QString GeoDataManager::targetDirectory() const
{
    return AppPaths::xrayResourceDir();
}

bool GeoDataManager::isTargetWritable(QString* errorMessage) const
{
    const QString directory = targetDirectory();
    if (directory.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Xray path is not configured.");
        }
        return false;
    }

    QDir dir(directory);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            if (errorMessage) {
                *errorMessage =
                    QStringLiteral("Cannot create Xray resource directory: %1").arg(directory);
            }
            return false;
        }
    }

    const QString probePath = dir.filePath(QStringLiteral(".zarya_write_test"));
    QFile probe(probePath);
    if (!probe.open(QIODevice::WriteOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Xray resource directory is not writable: %1")
                                .arg(directory);
        }
        return false;
    }
    probe.remove();
    return true;
}

QString GeoDataManager::settingsKeyForVerifiedHash(GeoDataKind kind) const
{
    switch (kind) {
    case GeoDataKind::GeoSite:
        return QStringLiteral("geodata/verified/geosite");
    case GeoDataKind::GeoIp:
        return QStringLiteral("geodata/verified/geoip");
    }
    return {};
}

QString GeoDataManager::verifiedHash(GeoDataKind kind) const
{
    return AppSettings::settings().value(settingsKeyForVerifiedHash(kind)).toString();
}

void GeoDataManager::setVerifiedHash(GeoDataKind kind, const QString& hash) const
{
    AppSettings::settings().setValue(settingsKeyForVerifiedHash(kind), hash.toLower());
}

GeoDataFileStatus GeoDataManager::checkFileStatus(GeoDataKind kind) const
{
    GeoDataFileStatus status;
    status.kind = kind;
    status.fileName = geoDataKindFileName(kind);
    status.path = QDir(targetDirectory()).filePath(status.fileName);

    QString writableError;
    if (!isTargetWritable(&writableError)) {
        status.status = GeoDataStatus::NotWritable;
        status.error = writableError;
        return status;
    }

    QFileInfo info(status.path);
    if (!info.exists() || !info.isFile()) {
        status.status = GeoDataStatus::Missing;
        return status;
    }

    status.sizeBytes = info.size();
    status.modifiedAt = info.lastModified();
    status.status = GeoDataStatus::Present;

    QString hashError;
    status.sha256 = GeoDataVerifier::sha256File(status.path, &hashError);
    if (status.sha256.isEmpty() && !hashError.isEmpty()) {
        status.error = hashError;
        return status;
    }

    const QString expected = verifiedHash(kind);
    status.expectedSha256 = expected;
    if (!expected.isEmpty() && !status.sha256.isEmpty()
        && status.sha256.compare(expected, Qt::CaseInsensitive) == 0) {
        status.status = GeoDataStatus::Verified;
    }

    return status;
}

QVector<GeoDataFileStatus> GeoDataManager::checkAllStatus() const
{
    return {checkFileStatus(GeoDataKind::GeoIp), checkFileStatus(GeoDataKind::GeoSite)};
}

bool GeoDataManager::profileNeedsGeoIp(const QStringList& geoTags) const
{
    for (const QString& tag : geoTags) {
        const QString lower = tag.trimmed().toLower();
        if (lower.startsWith(QStringLiteral("geoip:"))
            || lower.startsWith(QStringLiteral("ext:geoip.dat:"))) {
            return true;
        }
    }
    return false;
}

bool GeoDataManager::profileNeedsGeoSite(const QStringList& geoTags) const
{
    for (const QString& tag : geoTags) {
        const QString lower = tag.trimmed().toLower();
        if (lower.startsWith(QStringLiteral("geosite:"))
            || lower.startsWith(QStringLiteral("ext:geosite.dat:"))) {
            return true;
        }
    }
    return false;
}

bool GeoDataManager::hasRequiredFilesForTags(const QStringList& geoTags) const
{
    return missingFileNamesForTags(geoTags).isEmpty();
}

QStringList GeoDataManager::missingFileNamesForTags(const QStringList& geoTags) const
{
    QStringList missing;
    if (profileNeedsGeoIp(geoTags)) {
        const GeoDataFileStatus status = checkFileStatus(GeoDataKind::GeoIp);
        if (status.status == GeoDataStatus::Missing
            || status.status == GeoDataStatus::NotWritable) {
            missing.append(geoDataKindFileName(GeoDataKind::GeoIp));
        }
    }
    if (profileNeedsGeoSite(geoTags)) {
        const GeoDataFileStatus status = checkFileStatus(GeoDataKind::GeoSite);
        if (status.status == GeoDataStatus::Missing
            || status.status == GeoDataStatus::NotWritable) {
            missing.append(geoDataKindFileName(GeoDataKind::GeoSite));
        }
    }
    return missing;
}

bool GeoDataManager::cancel()
{
    m_cancelRequested.store(true);
    return true;
}

void GeoDataManager::updateGeoIp()
{
    startUpdate({GeoDataKind::GeoIp});
}

void GeoDataManager::updateGeoSite()
{
    startUpdate({GeoDataKind::GeoSite});
}

void GeoDataManager::updateAll()
{
    startUpdate({GeoDataKind::GeoIp, GeoDataKind::GeoSite});
}

void GeoDataManager::verifyAll()
{
    emit logLine(QStringLiteral("Checking geo data status"));
    const QVector<GeoDataFileStatus> statuses = checkAllStatus();
    emit statusChanged(statuses);
    emit updateFinished(true);
}

void GeoDataManager::startUpdate(const QVector<GeoDataKind>& kinds)
{
    if (m_updateThread && m_updateThread->isRunning()) {
        emit logLine(QStringLiteral("Geo data update already in progress"));
        return;
    }

    m_cancelRequested.store(false);
    if (m_updateThread) {
        m_updateThread->wait();
        delete m_updateThread;
        m_updateThread = nullptr;
    }

    m_updateThread = QThread::create([this, kinds]() { runUpdate(kinds); });
    connect(m_updateThread, &QThread::finished, this, [this]() {
        m_updateThread->deleteLater();
        m_updateThread = nullptr;
    });
    m_updateThread->start();
}

void GeoDataManager::runUpdate(const QVector<GeoDataKind>& kinds)
{
    const GeoDataSource source =
        GeoDataSources::sourceById(GeoDataSettingsStore::instance().selectedSourceId());
    const QString userAgent = userAgentString();
    bool allOk = true;

    emit logLine(QStringLiteral("Geo data target directory: %1").arg(targetDirectory()));

    for (GeoDataKind kind : kinds) {
        if (m_cancelRequested.load()) {
            allOk = false;
            break;
        }
        if (!updateFile(kind, source, userAgent)) {
            allOk = false;
        }
        if (m_cancelRequested.load()) {
            allOk = false;
            break;
        }
    }

    const QVector<GeoDataFileStatus> statuses = checkAllStatus();
    QMetaObject::invokeMethod(
        this, [this, statuses]() { emit statusChanged(statuses); }, Qt::QueuedConnection);
    QMetaObject::invokeMethod(
        this, [this, allOk]() { emit updateFinished(allOk); }, Qt::QueuedConnection);

    if (allOk) {
        QMetaObject::invokeMethod(
            this, [this]() { emit logLine(QStringLiteral("Geo data update completed")); },
            Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(
            this,
            [this]() { emit logLine(QStringLiteral("Geo data update failed or was canceled")); },
            Qt::QueuedConnection);
    }
}

bool GeoDataManager::updateFile(GeoDataKind kind, GeoDataSource source, const QString& userAgent)
{
    const QString fileName = geoDataKindFileName(kind);
    const auto log = [this](const QString& line) {
        QMetaObject::invokeMethod(
            this, [this, line]() { emit logLine(line); }, Qt::QueuedConnection);
    };

    log(QStringLiteral("Updating %1 from %2").arg(fileName, source.name));

    QString writableError;
    if (!isTargetWritable(&writableError)) {
        log(QStringLiteral("Geo data update failed: %1").arg(writableError));
        return false;
    }

    const QString directory = targetDirectory();
    const QString finalPath = QDir(directory).filePath(fileName);
    const QString tempPath = finalPath + QStringLiteral(".download");
    const QString backupPath = finalPath + QStringLiteral(".bak");

    GeoDataDownloader downloader;
    const auto cancelCallback = [this]() { return m_cancelRequested.load(); };
    const auto progressCallback = [this, kind](qint64 received, qint64 total) {
        QMetaObject::invokeMethod(
            this, [this, kind, received, total]() { emit progressChanged(kind, received, total); },
            Qt::QueuedConnection);
    };

    log(QStringLiteral("Downloading checksum"));
    const GeoDataDownloadResult checksumResult =
        downloader.download(urlForKind(source, kind, true), userAgent, {}, cancelCallback);
    if (!checksumResult.success) {
        log(QStringLiteral("Geo data update failed: %1").arg(checksumResult.errorMessage));
        return false;
    }

    const QString expectedSha256 =
        GeoDataVerifier::parseSha256Sum(checksumResult.body, fileName);
    if (expectedSha256.isEmpty()) {
        log(QStringLiteral("Geo data update failed: could not parse checksum for %1").arg(fileName));
        return false;
    }

    if (QFile::exists(tempPath) && !QFile::remove(tempPath)) {
        log(QStringLiteral("Geo data update failed: cannot remove stale temp file"));
        return false;
    }

    log(QStringLiteral("Downloading %1").arg(fileName));
    QString downloadError;
    if (!downloader.downloadToFile(urlForKind(source, kind, false), tempPath, userAgent,
                                   &downloadError, progressCallback, cancelCallback)) {
        QFile::remove(tempPath);
        log(QStringLiteral("Geo data update failed: %1").arg(downloadError));
        return false;
    }

    log(QStringLiteral("Verifying SHA256"));
    QString verifyError;
    if (!GeoDataVerifier::verifySha256(tempPath, expectedSha256, &verifyError)) {
        QFile::remove(tempPath);
        log(QStringLiteral("Geo data update failed: %1").arg(verifyError));
        return false;
    }
    log(QStringLiteral("SHA256 verified"));

    if (QFile::exists(finalPath)) {
        log(QStringLiteral("Backing up existing %1").arg(fileName));
        if (QFile::exists(backupPath) && !QFile::remove(backupPath)) {
            QFile::remove(tempPath);
            log(QStringLiteral("Geo data update failed: cannot replace backup file"));
            return false;
        }
        if (!QFile::rename(finalPath, backupPath)) {
            QFile::remove(tempPath);
            log(QStringLiteral("Geo data update failed: cannot backup existing file"));
            return false;
        }
    }

    log(QStringLiteral("Replacing %1").arg(fileName));
    if (!QFile::rename(tempPath, finalPath)) {
        if (QFile::exists(backupPath)) {
            QFile::remove(finalPath);
            QFile::rename(backupPath, finalPath);
        }
        QFile::remove(tempPath);
        log(QStringLiteral("Geo data update failed: cannot replace geo data file"));
        return false;
    }

    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }

    setVerifiedHash(kind, expectedSha256);
    return true;
}

} // namespace zarya
