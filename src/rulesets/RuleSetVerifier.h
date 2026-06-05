#pragma once

#include <QString>

namespace zarya {

class RuleSetVerifier {
public:
    static QString sha256OfFile(const QString& path, QString* errorMessage = nullptr);
    static bool verifyFileSha256(const QString& path, const QString& expectedSha256,
                               QString* errorMessage = nullptr);
};

} // namespace zarya
