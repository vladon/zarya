#include "helper/HelperAuth.h"

#include <QFile>

namespace zarya {

bool HelperAuth::loadTokenFromFile(const QString& tokenFilePath, QString* token,
                                   QString* errorMessage)
{
    QFile file(tokenFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot read token file: %1").arg(file.errorString());
        }
        return false;
    }
    const QString value = QString::fromUtf8(file.readAll()).trimmed();
    if (value.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Token file is empty.");
        }
        return false;
    }
    if (token) {
        *token = value;
    }
    return true;
}

} // namespace zarya
