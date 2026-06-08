#pragma once

#include "diagnostics/DiagnosticsOptions.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace zarya {

class DiagnosticsRedactor {
public:
    static QString redactPath(const QString& path, DiagnosticsRedactionMode mode,
                              bool includeMachinePaths);
    static QString redactText(const QString& text, DiagnosticsRedactionMode mode);
    static QString redactLogLine(const QString& line, DiagnosticsRedactionMode mode);
    static QJsonObject redactJsonObject(const QJsonObject& object, DiagnosticsRedactionMode mode);
    static QJsonObject redactProfileSummary(const struct Profile& profile,
                                            DiagnosticsRedactionMode mode);

    static QStringList redactedFieldReport();
    static void resetReport();
    static void trackField(const QString& field);

private:
    static QStringList s_redactedFields;
};

} // namespace zarya
