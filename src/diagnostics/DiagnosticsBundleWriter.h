#pragma once

#include "diagnostics/DiagnosticsOptions.h"
#include "diagnostics/DiagnosticsSnapshot.h"

#include <QString>

namespace zarya {

class DiagnosticsBundleWriter {
public:
    static bool write(const DiagnosticsSnapshot& snapshot, const DiagnosticsOptions& options,
                      const QString& outputPath, QString* error);
};

} // namespace zarya
