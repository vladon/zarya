#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace zarya {

struct DiagnosticsSnapshot {
    QHash<QString, QJsonObject> jsonFiles;
    QHash<QString, QString> textFiles;
    QStringList warnings;
    QStringList collectorErrors;
};

} // namespace zarya
