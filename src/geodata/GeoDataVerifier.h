#pragma once

#include <QByteArray>
#include <QString>

namespace zarya {

class GeoDataVerifier {
public:
    static QString sha256File(const QString& path, QString* errorMessage = nullptr);
    static QString parseSha256Sum(const QByteArray& content, const QString& expectedFileName);
    static bool verifySha256(const QString& filePath, const QString& expectedSha256,
                             QString* errorMessage = nullptr);
    static bool isValidSha256Hex(const QString& value);
};

} // namespace zarya
