#include "updater/runner/PortableUpdateApplier.h"

#include "platform/PlatformProcessUtils.h"
#include "updater/AppUpdatePaths.h"
#include "updater/AppVersion.h"
#include "updater/runner/UpdateRollback.h"
#include "storage/AppSettings.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>

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

bool copyTree(const QString& sourceRoot, const QString& destinationRoot,
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
                log->warning(
                    QStringLiteral("Skipping preserved path during copy: %1").arg(relative));
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
                log->error(QStringLiteral("Failed to copy %1").arg(relative));
            }
            return false;
        }
    }
    return true;
}

bool isProcessRunningByName(const QString& executableName)
{
#if defined(Q_OS_WIN)
    QProcess process;
    process.setProgram(QStringLiteral("tasklist"));
    process.setArguments({QStringLiteral("/FI"),
                          QStringLiteral("IMAGENAME eq %1").arg(executableName),
                          QStringLiteral("/NH")});
    process.start();
    if (!process.waitForFinished(10000)) {
        return false;
    }
    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    return output.contains(executableName, Qt::CaseInsensitive);
#else
    QProcess process;
    process.setProgram(QStringLiteral("pgrep"));
    process.setArguments({QStringLiteral("-x"), executableName});
    process.start();
    if (!process.waitForFinished(5000)) {
        return false;
    }
    return process.exitCode() == 0;
#endif
}

} // namespace

PortableUpdateApplier::PortableUpdateApplier(UpdaterLog& log)
    : m_log(log)
{
}

bool PortableUpdateApplier::isPreservedRelativePath(const QString& relativePath,
                                                    const QStringList& preservePaths)
{
    return ::zarya::isPreservedRelativePath(relativePath, preservePaths);
}

bool PortableUpdateApplier::removePathRecursively(const QString& path)
{
    return removeRecursively(path);
}

bool PortableUpdateApplier::copyPathRecursively(const QString& sourcePath,
                                                const QString& destinationPath,
                                                const QStringList& skipRelativePrefixes,
                                                const QString& sourceRoot, UpdaterLog* log)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destinationPath);
    return copyTree(sourceRoot, QFileInfo(destinationPath).absolutePath(), skipRelativePrefixes,
                    log);
}

bool PortableUpdateApplier::waitForMainProcessExit(const UpdatePlan& plan, int timeoutMs,
                                                   QString* errorMessage)
{
    m_log.info(QStringLiteral("Waiting for Zarya to exit"));
    const QString executableName = QFileInfo(plan.mainExecutable).fileName();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (!isProcessRunningByName(executableName)) {
            QThread::msleep(500);
            if (!isProcessRunningByName(executableName)) {
                m_log.info(QStringLiteral("Zarya process exited"));
                return true;
            }
        }
        QThread::msleep(500);
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Timed out waiting for Zarya to exit.");
    }
    m_log.error(QStringLiteral("Timed out waiting for Zarya to exit"));
    return false;
}

bool PortableUpdateApplier::createBackup(const UpdatePlan& plan, QString* errorMessage)
{
    m_log.info(QStringLiteral("Backing up app files"));
    QDir().mkpath(plan.backupDir);

    const QDir appDir(plan.appDir);
    const QStringList entries = appDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QString& entry : entries) {
        if (isPreservedRelativePath(entry, plan.preservePaths)) {
            continue;
        }
        const QString sourcePath = appDir.filePath(entry);
        const QString destinationPath = QDir(plan.backupDir).filePath(entry);
        if (QFileInfo(sourcePath).isDir()) {
            QDir().mkpath(destinationPath);
            if (!copyTree(sourcePath, destinationPath, plan.preservePaths, &m_log)) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Failed to back up %1").arg(entry);
                }
                return false;
            }
        } else {
            QDir().mkpath(QFileInfo(destinationPath).absolutePath());
            if (QFile::exists(destinationPath)) {
                QFile::remove(destinationPath);
            }
            if (!QFile::copy(sourcePath, destinationPath)) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Failed to back up %1").arg(entry);
                }
                return false;
            }
        }
    }
    return true;
}

bool PortableUpdateApplier::removeAppFilesExceptPreserved(const UpdatePlan& plan,
                                                          QString* errorMessage)
{
    const QDir appDir(plan.appDir);
    const QStringList entries = appDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QString& entry : entries) {
        if (isPreservedRelativePath(entry, plan.preservePaths)) {
            continue;
        }
        if (!removeRecursively(appDir.filePath(entry))) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to remove %1").arg(entry);
            }
            return false;
        }
    }
    return true;
}

bool PortableUpdateApplier::copyStagedFiles(const UpdatePlan& plan, QString* errorMessage)
{
    m_log.info(QStringLiteral("Copying new files"));
    if (!QDir(plan.stagingDir).exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staging directory does not exist.");
        }
        return false;
    }
    if (!copyTree(plan.stagingDir, plan.appDir, plan.preservePaths, &m_log)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to copy staged update files.");
        }
        return false;
    }
    return true;
}

bool PortableUpdateApplier::runPostUpdateCheck(const UpdatePlan& plan, QString* errorMessage)
{
    m_log.info(QStringLiteral("Running post-update check"));
    const QString program = QDir(plan.appDir).filePath(plan.postUpdateCheck.command);
    const ProcessResult result = runProcess(program, plan.postUpdateCheck.args, 30000);
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = result.errorMessage.isEmpty()
                                ? QStringLiteral("Post-update check failed.")
                                : result.errorMessage;
        }
        return false;
    }

    const QRegularExpression versionPattern(
        QStringLiteral("Zarya\\s+(\\S+)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = versionPattern.match(result.standardOutput);
    const QString detectedVersion =
        match.hasMatch() ? match.captured(1) : result.standardOutput.trimmed();
    if (AppVersion::compare(detectedVersion, plan.postUpdateCheck.expectedVersion) != 0) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("Post-update version mismatch. Expected %1, got %2.")
                    .arg(plan.postUpdateCheck.expectedVersion, detectedVersion);
        }
        return false;
    }
    return true;
}

bool PortableUpdateApplier::restartApplication(const UpdatePlan& plan, QString* errorMessage)
{
    if (!plan.restartAfterUpdate) {
        return true;
    }
    const QString program = QDir(plan.appDir).filePath(plan.mainExecutable);
    const bool started = QProcess::startDetached(program, plan.restartArgs, plan.appDir);
    if (!started && errorMessage) {
        *errorMessage = QStringLiteral("Failed to restart Zarya.");
    }
    return started;
}

void PortableUpdateApplier::writeStateFile(const UpdatePlan& plan, const QString& fileName,
                                           const QJsonObject& extraFields)
{
    QJsonObject object = extraFields;
    object.insert(QStringLiteral("targetVersion"), plan.targetVersion);
    object.insert(QStringLiteral("previousVersion"), plan.currentVersion);
    object.insert(QStringLiteral("completedAt"),
                  QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    object.insert(QStringLiteral("backupDir"), plan.backupDir);
    object.insert(QStringLiteral("stagingDir"), plan.stagingDir);

    const QString path = QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(fileName);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    }

    const QString lastLogPath =
        QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(QStringLiteral("last-update.log"));
    if (QFile::exists(m_log.logFilePath())) {
        if (QFile::exists(lastLogPath)) {
            QFile::remove(lastLogPath);
        }
        QFile::copy(m_log.logFilePath(), lastLogPath);
    }
}

void PortableUpdateApplier::pruneOldBackups(const UpdatePlan& plan, int retentionCount)
{
    if (retentionCount <= 0) {
        return;
    }
    QDir backupsRoot(QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(QStringLiteral("backups")));
    if (!backupsRoot.exists()) {
        return;
    }
    const QFileInfoList entries =
        backupsRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    int index = 0;
    for (const QFileInfo& entry : entries) {
        ++index;
        if (index <= retentionCount) {
            continue;
        }
        if (entry.absoluteFilePath() == plan.backupDir) {
            continue;
        }
        QDir(entry.absoluteFilePath()).removeRecursively();
    }
}

bool PortableUpdateApplier::apply(const UpdatePlan& plan, QString* errorMessage)
{
    m_log.info(QStringLiteral("Update started"));
    m_log.info(QStringLiteral("Current version: %1").arg(plan.currentVersion));
    m_log.info(QStringLiteral("Target version: %1").arg(plan.targetVersion));

    QString validationError;
    if (!plan.isValid(&validationError)) {
        if (errorMessage) {
            *errorMessage = validationError;
        }
        m_log.error(validationError);
        return false;
    }

    if (!waitForMainProcessExit(plan, 60000, errorMessage)) {
        return false;
    }

    if (!createBackup(plan, errorMessage)) {
        return false;
    }

    if (!removeAppFilesExceptPreserved(plan, errorMessage)) {
        return false;
    }

    if (!copyStagedFiles(plan, errorMessage)) {
        return false;
    }

    QString checkError;
    if (!runPostUpdateCheck(plan, &checkError)) {
        m_log.error(checkError);
        UpdateRollback rollback(m_log);
        if (!rollback.rollback(plan, &checkError)) {
            const QString criticalPath = QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(
                QStringLiteral("update-critical-failure.txt"));
            QFile critical(criticalPath);
            if (critical.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                critical.write(checkError.toUtf8());
            }
            if (errorMessage) {
                *errorMessage = checkError;
            }
            return false;
        }
        writeStateFile(plan, QStringLiteral("update-failed.json"),
                       {{QStringLiteral("reason"), checkError}});
        rollback.restartPreviousVersion(plan, {QStringLiteral("--update-rollback")});
        if (errorMessage) {
            *errorMessage = checkError;
        }
        return false;
    }

    m_log.info(QStringLiteral("Update succeeded"));
    writeStateFile(plan, QStringLiteral("update-success.json"), {});
    pruneOldBackups(plan, AppSettings::instance().appUpdateBackupRetentionCount());

    QFile pendingFile(AppUpdatePaths::pendingUpdatePath());
    if (pendingFile.exists()) {
        pendingFile.remove();
    }

    if (!restartApplication(plan, errorMessage)) {
        return false;
    }
    return true;
}

} // namespace zarya
