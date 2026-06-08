#include "cores/CoreVerifier.h"

#include "cores/CoreVersionDetector.h"

#include <QDirIterator>
#include <QFileInfo>

namespace zarya {

QString CoreVerifier::executableFileName(CoreType type)
{
#ifdef Q_OS_WIN
    return type == CoreType::Xray ? QStringLiteral("xray.exe") : QStringLiteral("sing-box.exe");
#else
    return type == CoreType::Xray ? QStringLiteral("xray") : QStringLiteral("sing-box");
#endif
}

QString CoreVerifier::findExecutableInTree(const QString& rootDir, CoreType type)
{
    const QString targetName = executableFileName(type);
    QDirIterator iterator(rootDir, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        if (!info.isFile()) {
            continue;
        }
        if (info.fileName().compare(targetName, Qt::CaseInsensitive) == 0) {
            return info.absoluteFilePath();
        }
    }
    return {};
}

bool CoreVerifier::verifyStaged(const QString& executablePath, CoreType type,
                                const QString& expectedVersion, QString* errorMessage)
{
    if (executablePath.isEmpty() || !QFileInfo::exists(executablePath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged executable not found.");
        }
        return false;
    }

    const DetectedVersion detected = CoreVersionDetector::detect(executablePath, type);
    if (!detected.ok) {
        if (errorMessage) {
            *errorMessage = detected.error.isEmpty() ? QStringLiteral("Version check failed.")
                                                       : detected.error;
        }
        return false;
    }

    const QString normalizedExpected = CoreVersionDetector::normalizeVersion(expectedVersion);
    const QString normalizedDetected = CoreVersionDetector::normalizeVersion(detected.version);
    if (!normalizedExpected.isEmpty() && !normalizedDetected.isEmpty()
        && normalizedDetected != normalizedExpected
        && !normalizedDetected.startsWith(normalizedExpected.section(QLatin1Char('.'), 0, 1))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Staged version %1 does not match expected %2.")
                                .arg(normalizedDetected, normalizedExpected);
        }
        return false;
    }

    return true;
}

} // namespace zarya
