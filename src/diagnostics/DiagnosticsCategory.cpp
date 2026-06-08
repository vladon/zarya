#include "diagnostics/DiagnosticsCategory.h"

namespace zarya {

QString diagnosticsCategoryKey(DiagnosticsCategory category)
{
    switch (category) {
    case DiagnosticsCategory::AppInfo:
        return QStringLiteral("appInfo");
    case DiagnosticsCategory::PlatformInfo:
        return QStringLiteral("platformInfo");
    case DiagnosticsCategory::Paths:
        return QStringLiteral("paths");
    case DiagnosticsCategory::RuntimeStatus:
        return QStringLiteral("runtimeStatus");
    case DiagnosticsCategory::HelperStatus:
        return QStringLiteral("helperStatus");
    case DiagnosticsCategory::SystemProxyStatus:
        return QStringLiteral("systemProxy");
    case DiagnosticsCategory::KillSwitchStatus:
        return QStringLiteral("killSwitch");
    case DiagnosticsCategory::CoreStatus:
        return QStringLiteral("coreStatus");
    case DiagnosticsCategory::RoutingDnsStatus:
        return QStringLiteral("routingDns");
    case DiagnosticsCategory::GeoDataStatus:
        return QStringLiteral("geoData");
    case DiagnosticsCategory::RuleSetStatus:
        return QStringLiteral("ruleSets");
    case DiagnosticsCategory::GeneratedConfigPreview:
        return QStringLiteral("configPreview");
    case DiagnosticsCategory::ValidationOutput:
        return QStringLiteral("validation");
    case DiagnosticsCategory::RecentLogs:
        return QStringLiteral("logs");
    case DiagnosticsCategory::RecentErrors:
        return QStringLiteral("recentErrors");
    }
    return {};
}

QString diagnosticsCategoryDisplayName(DiagnosticsCategory category)
{
    switch (category) {
    case DiagnosticsCategory::AppInfo:
        return QStringLiteral("App/platform info");
    case DiagnosticsCategory::PlatformInfo:
        return QStringLiteral("Platform info");
    case DiagnosticsCategory::Paths:
        return QStringLiteral("Paths");
    case DiagnosticsCategory::RuntimeStatus:
        return QStringLiteral("Runtime status");
    case DiagnosticsCategory::HelperStatus:
        return QStringLiteral("Helper status");
    case DiagnosticsCategory::SystemProxyStatus:
        return QStringLiteral("System proxy status");
    case DiagnosticsCategory::KillSwitchStatus:
        return QStringLiteral("Kill switch status");
    case DiagnosticsCategory::CoreStatus:
        return QStringLiteral("Core versions");
    case DiagnosticsCategory::RoutingDnsStatus:
        return QStringLiteral("Routing/DNS status");
    case DiagnosticsCategory::GeoDataStatus:
        return QStringLiteral("Geo data status");
    case DiagnosticsCategory::RuleSetStatus:
        return QStringLiteral("Rule-set status");
    case DiagnosticsCategory::GeneratedConfigPreview:
        return QStringLiteral("Redacted config previews");
    case DiagnosticsCategory::ValidationOutput:
        return QStringLiteral("Validation output");
    case DiagnosticsCategory::RecentLogs:
        return QStringLiteral("Recent redacted logs");
    case DiagnosticsCategory::RecentErrors:
        return QStringLiteral("Recent errors");
    }
    return {};
}

QVector<DiagnosticsCategory> defaultDiagnosticsCategories()
{
    return {DiagnosticsCategory::AppInfo,
            DiagnosticsCategory::PlatformInfo,
            DiagnosticsCategory::Paths,
            DiagnosticsCategory::RuntimeStatus,
            DiagnosticsCategory::HelperStatus,
            DiagnosticsCategory::SystemProxyStatus,
            DiagnosticsCategory::KillSwitchStatus,
            DiagnosticsCategory::CoreStatus,
            DiagnosticsCategory::RoutingDnsStatus,
            DiagnosticsCategory::GeoDataStatus,
            DiagnosticsCategory::RuleSetStatus,
            DiagnosticsCategory::GeneratedConfigPreview,
            DiagnosticsCategory::ValidationOutput,
            DiagnosticsCategory::RecentLogs,
            DiagnosticsCategory::RecentErrors};
}

} // namespace zarya
