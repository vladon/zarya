#pragma once

#include "cores/CoreAsset.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>

namespace zarya {

class CoreChecksum {
public:
    static std::optional<QString> findChecksumAssetName(const QVector<CoreAsset>& assets,
                                                        const QString& archiveName);
    static std::optional<QString> parseExpectedSha256(const QByteArray& checksumBody,
                                                      const QString& archiveName);
    static QString sha256OfFile(const QString& filePath, QString* errorMessage = nullptr);
    static bool verifyFileSha256(const QString& filePath, const QString& expectedSha256,
                                 QString* errorMessage = nullptr);
};

} // namespace zarya
