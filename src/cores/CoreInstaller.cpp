#include "cores/CoreInstaller.h"

#include "cores/CorePaths.h"
#include "cores/CoreRollbackManager.h"
#include "cores/CoreVerifier.h"
#include "cores/CoreVersionDetector.h"
#include "domain/CoreType.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

bool copyExecutable(const QString& source, const QString& destination, QString* errorMessage)
{
    if (QFile::exists(destination) && !QFile::remove(destination)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not replace existing executable.");
        }
        return false;
    }
    if (!QFile::copy(source, destination)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not copy executable to install directory.");
        }
        return false;
    }
    QFile::setPermissions(destination, QFile::permissions(source) | QFileDevice::ExeUser
                                                         | QFileDevice::ExeGroup
                                                         | QFileDevice::ExeOther);
    return true;
}

} // namespace

bool CoreInstaller::installFromStaging(CoreType type, const QString& stagedExecutablePath,
                                       const QString& installDir, const QString& version,
                                       QString* errorMessage)
{
    QDir().mkpath(installDir);

    const QString destinationExecutable =
        QDir(installDir).filePath(CoreVerifier::executableFileName(type));

    QString installedVersion;
    if (QFile::exists(destinationExecutable)) {
        const auto detected = CoreVersionDetector::detect(destinationExecutable, type);
        installedVersion = detected.version;
    }

    if (!CoreRollbackManager::createBackup(type, installDir, installedVersion, errorMessage)) {
        return false;
    }

    if (!copyExecutable(stagedExecutablePath, destinationExecutable, errorMessage)) {
        return false;
    }

    QFile versionFile(CorePaths::versionFilePath(installDir));
    if (versionFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        versionFile.write(version.toUtf8());
    }

    QJsonObject metadata{
        {QStringLiteral("version"), version},
        {QStringLiteral("coreType"), coreTypeToString(type)},
        {QStringLiteral("installedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
    QFile metadataFile(CorePaths::metadataFilePath(installDir));
    if (metadataFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        metadataFile.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));
    }

    if (!CoreVerifier::verifyStaged(destinationExecutable, type, version, errorMessage)) {
        QString restoredVersion;
        CoreRollbackManager::restoreLatestBackup(type, installDir, &restoredVersion, nullptr);
        return false;
    }

    return true;
}

} // namespace zarya
