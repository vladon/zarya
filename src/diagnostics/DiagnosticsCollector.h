#pragma once

#include "diagnostics/DiagnosticsContext.h"
#include "diagnostics/DiagnosticsOptions.h"
#include "diagnostics/DiagnosticsSnapshot.h"

#include <functional>
#include <QString>

namespace zarya {

class DiagnosticsCollector {
public:
    using LogCallback = std::function<void(const QString&)>;

    static DiagnosticsSnapshot collect(const DiagnosticsOptions& options,
                                       const DiagnosticsContext& context,
                                       LogCallback log = {});
};

} // namespace zarya
