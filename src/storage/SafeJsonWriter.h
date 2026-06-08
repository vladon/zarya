#pragma once

#include <QJsonDocument>
#include <QString>

namespace zarya {

class SafeJsonWriter {
public:
    static bool writeDocument(const QString& filePath, const QJsonDocument& document,
                              QString* errorMessage = nullptr);
    static bool createBackup(const QString& filePath, QString* errorMessage = nullptr);
};

} // namespace zarya
