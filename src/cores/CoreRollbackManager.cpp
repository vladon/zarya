#include "cores/CoreRollbackManager.h"

#include "domain/CoreType.h"
#include "storage/AppPaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace zarya {

namespace {

QString coreDirName(CoreType type)
{
    return type == CoreType::Xray ? QStringLiteral("xray") : QStringLiteral("sing-box");
}

bool copyFilePreserve(const QString& source, const QString& destination, QString* errorMessage)
{
    if (QFile::exists(destination) && !QFile::remove(destination)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not replace %1").arg(destination);
        }
        return false;
    }
    if (!QFile::copy(source, destination)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not copy %1 to %2").arg(source, destination);
        }
        return false;
    }
    return true;
}

} // namespace

QString CoreRollbackManager::backupRootDir()
{
    const QString path = QDir(AppPaths::coresDir()).filePath(QStringLiteral(".backup"));
    QDir().mkpath(path);
    return path;
}

QStringList CoreRollbackManager::listBackups(CoreType type)
{
    const QDir dir(backupRootDir());
    const QString prefix = coreDirName(type) + QLatin1Char('-');
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList result;
    for (const QString& entry : entries) {
        if (entry.startsWith(prefix)) {
            result.append(dir.filePath(entry));
        }
    }
    result.sort();
    return result;
}

bool CoreRollbackManager::createBackup(CoreType type, const QString& installDir,
                                       const QString& version, QString* errorMessage)
{
    const QString executableName = type == CoreType::Xray
                                         ? (QStringLiteral("xray")
#ifdef Q_OS_WIN
                                            + QStringLiteral(".exe")
#endif
                                            )
                                         : (QStringLiteral("sing-box")
#ifdef Q_OS_WIN
                                            + QStringLiteral(".exe")
#endif
                                            );

    const QString sourceExecutable = QDir(installDir).filePath(executableName);
    if (!QFile::exists(sourceExecutable)) {
        return true;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString safeVersion = version.isEmpty() ? QStringLiteral("unknown") : version;
    const QString backupDir =
        QDir(backupRootDir()).filePath(QStringLiteral("%1-%2-%3").arg(coreDirName(type), safeVersion, timestamp));
    QDir().mkpath(backupDir);

    const QStringList preserveFiles = type == CoreType::Xray
                                          ? QStringList{executableName, QStringLiteral("VERSION"),
                                                        QStringLiteral(".zarya-core.json")}
                                          : QStringList{executableName, QStringLiteral("VERSION"),
                                                        QStringLiteral(".zarya-core.json")};

    for (const QString& fileName : preserveFiles) {
        const QString source = QDir(installDir).filePath(fileName);
        if (!QFile::exists(source)) {
            continue;
        }
        if (!copyFilePreserve(source, QDir(backupDir).filePath(fileName), errorMessage)) {
            return false;
        }
    }
    return true;
}

bool CoreRollbackManager::restoreLatestBackup(CoreType type, const QString& installDir,
                                              QString* restoredVersion, QString* errorMessage)
{
    const QStringList backups = listBackups(type);
    if (backups.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No backup available for rollback.");
        }
        return false;
    }

    const QString backupDir = backups.last();
    QDir backup(backupDir);
    const QStringList files = backup.entryList(QDir::Files);
    for (const QString& fileName : files) {
        if (!copyFilePreserve(backup.filePath(fileName), QDir(installDir).filePath(fileName),
                              errorMessage)) {
            return false;
        }
    }

    QFile versionFile(backup.filePath(QStringLiteral("VERSION")));
    if (versionFile.open(QIODevice::ReadOnly) && restoredVersion) {
        *restoredVersion = QString::fromUtf8(versionFile.readAll()).trimmed();
    }
    return true;
}

void CoreRollbackManager::pruneBackups(CoreType type, int retentionCount)
{
    if (retentionCount < 1) {
        return;
    }
    QStringList backups = listBackups(type);
    while (backups.size() > retentionCount) {
        const QString oldest = backups.takeFirst();
        QDir(oldest).removeRecursively();
    }
}

} // namespace zarya
