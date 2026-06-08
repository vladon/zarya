#include "diagnostics/DiagnosticsManager.h"

#include "diagnostics/DiagnosticsBundleWriter.h"
#include "diagnostics/DiagnosticsCollector.h"

#include <QDateTime>
#include <QDir>

namespace zarya {

DiagnosticsManager::DiagnosticsManager(QObject* parent)
    : QObject(parent)
{
}

void DiagnosticsManager::setContext(const DiagnosticsContext& context)
{
    m_context = context;
}

DiagnosticsPreviewResult DiagnosticsManager::buildPreview(const DiagnosticsOptions& options) const
{
    const DiagnosticsSnapshot snapshot = DiagnosticsCollector::collect(options, m_context, {});

    DiagnosticsPreviewResult result;
    result.redactionMode =
        options.redactionMode == DiagnosticsRedactionMode::Strict ? QStringLiteral("Strict")
                                                                  : QStringLiteral("Basic");
    result.secretsIncluded = false;
    result.warnings = snapshot.warnings;

    for (auto it = snapshot.jsonFiles.constBegin(); it != snapshot.jsonFiles.constEnd(); ++it) {
        result.files.append(it.key());
    }
    for (auto it = snapshot.textFiles.constBegin(); it != snapshot.textFiles.constEnd(); ++it) {
        result.files.append(it.key());
    }
    result.files.append(QStringLiteral("manifest.json"));
    result.files.sort();
    return result;
}

bool DiagnosticsManager::createBundle(const DiagnosticsOptions& options, QString* outputPath,
                                      QString* error)
{
    QString path = options.outputPath;
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/zarya-diagnostics-")
               + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
               + QStringLiteral(".zarya-diagnostics.zip");
    }
    if (!path.endsWith(QStringLiteral(".zarya-diagnostics.zip"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".zarya-diagnostics.zip");
    }

    emit logLine(QStringLiteral("Diagnostics bundle export started"));
    const DiagnosticsSnapshot snapshot =
        DiagnosticsCollector::collect(options, m_context, [this](const QString& line) {
            emit logLine(line);
        });

    if (!DiagnosticsBundleWriter::write(snapshot, options, path, error)) {
        return false;
    }

    emit logLine(QStringLiteral("Diagnostics bundle created: %1").arg(path));
    if (outputPath) {
        *outputPath = path;
    }
    return true;
}

} // namespace zarya
