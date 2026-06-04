#include "storage/HelperSession.h"

#include "storage/AppPaths.h"

#include <QByteArray>
#include <QFile>
#include <QRandomGenerator>
#include <QSaveFile>

namespace zarya {

QString HelperSession::tokenFilePath()
{
    return AppPaths::helperTokenPath();
}

QString HelperSession::ensureSessionToken(QString* errorMessage)
{
    const QString path = tokenFilePath();
    QFile existing(path);
    if (existing.exists() && existing.open(QIODevice::ReadOnly)) {
        const QString token = QString::fromUtf8(existing.readAll()).trimmed();
        if (!token.isEmpty()) {
            return token;
        }
    }

    const QByteArray random = QByteArray::fromHex(
        QByteArray::number(QRandomGenerator::global()->generate64(), 16)
        + QByteArray::number(QRandomGenerator::global()->generate64(), 16));

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }
    const QString token = QString::fromLatin1(random.toHex());
    file.write(token.toUtf8());
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }

#if defined(Q_OS_UNIX)
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
#endif

    return token;
}

QString HelperSession::readSessionToken(const QString& tokenPath, QString* errorMessage)
{
    QFile file(tokenPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace zarya
