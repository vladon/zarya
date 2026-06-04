#pragma once

#include <QString>

namespace zarya {

class HelperAuth {
public:
    static bool loadTokenFromFile(const QString& tokenFilePath, QString* token,
                                  QString* errorMessage = nullptr);
};

} // namespace zarya
