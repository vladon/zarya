#include "rulesets/RuleSetVerifier.h"

#include <QCryptographicHash>
#include <QFile>

namespace zarya {

QString RuleSetVerifier::sha256OfFile(const QString& path, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return {};
    }
  return QString::fromLatin1(
        QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex());
}

bool RuleSetVerifier::verifyFileSha256(const QString& path, const QString& expectedSha256,
                                     QString* errorMessage)
{
    if (expectedSha256.trimmed().isEmpty()) {
        return true;
    }
    const QString actual = sha256OfFile(path, errorMessage);
    if (actual.isEmpty()) {
        return false;
    }
    if (actual.compare(expectedSha256.trimmed(), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Checksum mismatch for %1.").arg(path);
        }
        return false;
    }
    return true;
}

} // namespace zarya
