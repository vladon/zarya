#pragma once

#include "diagnostics/DiagnosticsCategory.h"

#include <QSet>
#include <QString>

namespace zarya {

enum class DiagnosticsRedactionMode {
    Basic,
    Strict,
};

struct DiagnosticsOptions {
    DiagnosticsRedactionMode redactionMode = DiagnosticsRedactionMode::Strict;
    QSet<DiagnosticsCategory> categories;
    bool includeMachinePaths = false;
    bool includeEnvironmentSummary = false;
    bool extendedLogs = false;
    bool runConfigValidation = true;
    QString outputPath;
};

} // namespace zarya
