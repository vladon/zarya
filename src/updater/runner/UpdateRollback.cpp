#include "updater/runner/UpdateRollback.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace zarya {

namespace {

bool isPreservedRelativePath(const QString& relativePath, const QStringList& preservePaths)
{
    const QString normalized = QDir::fromNativeSeparators(relativePath);
    for (const QString& preserve : preservePaths) {
        const QString preserveNormalized = QDir::fromNativeSeparators(preserve);
        if (normalized == preserveNormalized
            || normalized.startsWith(preserveNormalized + QLatin1Char('/'))) {
            return true;
        }
    }
    return false;
}

bool removeRecursively(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return true;
    }
    if (info.isDir()) {
        return QDir(path).removeRecursively();
    }
    return QFile::remove(path);
}

} // namespace

UpdateRollback::UpdateRollback(UpdaterLog& log)
    : m_log(log)
{
}

bool UpdateRollback::copyTree(const QString& sourceRoot, const QString& destinationRoot,
                              const QStringList& preservePaths, UpdaterLog* log)
{
    QDirIterator iterator(sourceRoot, QDir::NoDotAndDotDot | QDir::AllEntries,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QString sourcePath = iterator.filePath();
        const QString relative = QDir(sourceRoot).relativeFilePath(sourcePath);
        if (isPreservedRelativePath(relative, preservePaths)) {
            if (log) {
                log->warning(QStringLiteral("Skipping preserved path during rollback copy: %1")
                                 .arg(relative));
            }
            continue;
        }
        const QString destinationPath = QDir(destinationRoot).filePath(relative);
        if (iterator.fileInfo().isDir()) {
            QDir().mkpath(destinationPath);
            continue;
        }
        QDir().mkpath(QFileInfo(destinationPath).absolutePath());
        if (QFile::exists(destinationPath)) {
            QFile::remove(destinationPath);
        }
        if (!QFile::copy(sourcePath, destinationPath)) {
            if (log) {
                log->error(QStringLiteral("Failed to restore %1").arg(relative));
            }
            return false;
        }
    }
    return true;
}

bool UpdateRollback::rollback(const UpdatePlan& plan, QString* errorMessage)
{
    m_log.info(QStringLiteral("Rollback started"));

    const QDir appDir(plan.appDir);
    const QStringList entries = appDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QString& entry : entries) {
        if (isPreservedRelativePath(entry, plan.preservePaths)) {
            continue;
        }
        if (!removeRecursively(appDir.filePath(entry))) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to remove updated file %1").arg(entry);
            }
            return false;
        }
    }

    if (!QDir(plan.backupDir).exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Backup directory is missing.");
        }
        return false;
    }

    if (!copyTree(plan.backupDir, plan.appDir, plan.preservePaths, &m_log)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to restore backup files.");
        }
        return false;
    }

    m_log.info(QStringLiteral("Rollback completed"));
    return true;
}

bool UpdateRollback::restartPreviousVersion(const UpdatePlan& plan, const QStringList& extraArgs)
{
    const QString program = QDir(plan.appDir).filePath(plan.mainExecutable);
    return QProcess::startDetached(program, extraArgs, plan.appDir);
}

} // namespace zarya
