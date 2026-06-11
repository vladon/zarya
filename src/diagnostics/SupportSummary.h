#pragma once

#include "diagnostics/DiagnosticsContext.h"

#include <QString>

namespace zarya {

class SupportSummary {
public:
    static QString buildClipboardText(const DiagnosticsContext& context);
    static bool copyToClipboard(const DiagnosticsContext& context);
};

} // namespace zarya
