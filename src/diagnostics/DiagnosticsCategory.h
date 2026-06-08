#pragma once

#include <QString>
#include <QVector>

namespace zarya {

enum class DiagnosticsCategory {
    AppInfo,
    PlatformInfo,
    Paths,
    RuntimeStatus,
    HelperStatus,
    SystemProxyStatus,
    KillSwitchStatus,
    CoreStatus,
    RoutingDnsStatus,
    GeoDataStatus,
    RuleSetStatus,
    GeneratedConfigPreview,
    ValidationOutput,
    RecentLogs,
    RecentErrors,
};

QString diagnosticsCategoryKey(DiagnosticsCategory category);
QString diagnosticsCategoryDisplayName(DiagnosticsCategory category);
QVector<DiagnosticsCategory> defaultDiagnosticsCategories();

} // namespace zarya
