#pragma once

#include "updater/AppUpdateAsset.h"

#include <QString>

namespace zarya {

class AppUpdateVerifier {
public:
    static bool verifySha256(const QString& filePath, const QString& expectedHash,
                             QString* errorMessage = nullptr);
    static QString sha256OfFile(const QString& filePath, QString* errorMessage = nullptr);
    static QStringList signatureWarnings(const AppUpdateAsset& asset);
    static bool canDownloadAsset(const AppUpdateAsset& asset, bool allowUnsigned);
};

} // namespace zarya
