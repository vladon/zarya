#include "storage/SafeJsonWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace zarya {

bool SafeJsonWriter::createBackup(const QString& filePath, QString* errorMessage)
{
    if (!QFile::exists(filePath)) {
        return true;
    }
    const QString backupPath = filePath + QStringLiteral(".bak");
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    if (!QFile::copy(filePath, backupPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to create backup for %1").arg(filePath);
        }
        return false;
    }
    return true;
}

bool SafeJsonWriter::writeDocument(const QString& filePath, const QJsonDocument& document,
                                   QString* errorMessage)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

} // namespace zarya
