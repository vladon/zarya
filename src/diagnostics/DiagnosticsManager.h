#pragma once

#include "diagnostics/DiagnosticsContext.h"
#include "diagnostics/DiagnosticsOptions.h"
#include "diagnostics/DiagnosticsSnapshot.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace zarya {

struct DiagnosticsPreviewResult {
    QStringList files;
    QStringList warnings;
    QString redactionMode;
    bool secretsIncluded = false;
};

class DiagnosticsManager : public QObject {
    Q_OBJECT

public:
    explicit DiagnosticsManager(QObject* parent = nullptr);

    void setContext(const DiagnosticsContext& context);

    DiagnosticsPreviewResult buildPreview(const DiagnosticsOptions& options) const;
    bool createBundle(const DiagnosticsOptions& options, QString* outputPath, QString* error);

signals:
    void logLine(const QString& line);

private:
    DiagnosticsContext m_context;
};

} // namespace zarya
