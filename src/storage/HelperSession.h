#pragma once

#include <QString>

namespace zarya {

class HelperSession {
public:
    static QString ensureSessionToken(QString* errorMessage = nullptr);
    static QString readSessionToken(const QString& tokenPath, QString* errorMessage = nullptr);
    static QString tokenFilePath();
};

} // namespace zarya
