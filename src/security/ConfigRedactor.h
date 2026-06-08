#pragma once

#include <QJsonObject>
#include <QString>

namespace zarya {

enum class RedactionStrength {
    Basic,
    Strict,
};

class ConfigRedactor {
public:
    static QString redactText(const QString& text, RedactionStrength strength);
    static QString redactLogLine(const QString& line, RedactionStrength strength);
    static QJsonObject redactJsonObject(const QJsonObject& object, RedactionStrength strength);
    static QString redactPath(const QString& path, RedactionStrength strength,
                              bool includeMachinePaths = false);
};

} // namespace zarya
