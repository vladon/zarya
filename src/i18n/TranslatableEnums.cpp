#include "i18n/TranslatableEnums.h"

#include <QCoreApplication>

namespace zarya {

namespace {

QString trEnum(const char* text)
{
    return QCoreApplication::translate("Enums", text);
}

} // namespace

QString TranslatableEnums::trProtocolType(ProtocolType type)
{
    switch (type) {
    case ProtocolType::Vless:
        return QStringLiteral("VLESS");
    case ProtocolType::Vmess:
        return QStringLiteral("VMess");
    case ProtocolType::Trojan:
        return QStringLiteral("Trojan");
    case ProtocolType::Shadowsocks:
        return QStringLiteral("Shadowsocks");
    }
    return trEnum("Unknown");
}

QString TranslatableEnums::trCoreType(CoreType type)
{
    switch (type) {
    case CoreType::Xray:
        return QStringLiteral("Xray");
    case CoreType::SingBox:
        return QStringLiteral("sing-box");
    }
    return QStringLiteral("Xray");
}

QString TranslatableEnums::trRuntimeMode(RuntimeMode mode)
{
    switch (mode) {
    case RuntimeMode::SystemProxyXray:
        return trEnum("System proxy via Xray");
    case RuntimeMode::TunSingBoxExperimental:
        return trEnum("Experimental TUN via sing-box");
    }
    return trEnum("System proxy via Xray");
}

QString TranslatableEnums::trRuntimeState(RuntimeState state)
{
    switch (state) {
    case RuntimeState::Stopped:
        return trEnum("Stopped");
    case RuntimeState::Starting:
        return trEnum("Starting");
    case RuntimeState::Running:
        return trEnum("Running");
    case RuntimeState::Stopping:
        return trEnum("Stopping");
    case RuntimeState::Failed:
        return trEnum("Failed");
    case RuntimeState::Recovering:
        return trEnum("Recovering");
    }
    return trEnum("Stopped");
}

QString TranslatableEnums::trSubscriptionStatus(SubscriptionStatus status)
{
    switch (status) {
    case SubscriptionStatus::NeverUpdated:
        return trEnum("Never updated");
    case SubscriptionStatus::Updating:
        return trEnum("Updating");
    case SubscriptionStatus::Success:
        return trEnum("Success");
    case SubscriptionStatus::Failed:
        return trEnum("Failed");
    case SubscriptionStatus::Disabled:
        return trEnum("Disabled");
    }
    return trEnum("Never updated");
}

QString TranslatableEnums::trTestStatus(TestStatus status)
{
    switch (status) {
    case TestStatus::NeverTested:
        return trEnum("Never tested");
    case TestStatus::Testing:
        return trEnum("Testing");
    case TestStatus::Available:
        return trEnum("Available");
    case TestStatus::Timeout:
        return trEnum("Timeout");
    case TestStatus::Failed:
        return trEnum("Failed");
    case TestStatus::Unsupported:
        return trEnum("Unsupported");
    case TestStatus::Canceled:
        return trEnum("Canceled");
    }
    return trEnum("Never tested");
}

QString TranslatableEnums::trRoutingMode(RoutingMode mode)
{
    switch (mode) {
    case RoutingMode::ProxyAll:
        return trEnum("Proxy All");
    case RoutingMode::BypassLan:
        return trEnum("Bypass LAN");
    case RoutingMode::BypassRu:
        return trEnum("Bypass RU");
    case RoutingMode::BypassLanAndRu:
        return trEnum("Bypass LAN + RU");
    case RoutingMode::Custom:
        return trEnum("Custom");
    }
    return trEnum("Custom");
}

QString TranslatableEnums::trRoutingAction(RoutingAction action)
{
    switch (action) {
    case RoutingAction::Proxy:
        return trEnum("Proxy");
    case RoutingAction::Direct:
        return trEnum("Direct");
    case RoutingAction::Block:
        return trEnum("Block");
    }
    return trEnum("Proxy");
}

QString TranslatableEnums::trDnsProfileMode(DnsProfileMode mode)
{
    switch (mode) {
    case DnsProfileMode::System:
        return trEnum("System DNS");
    case DnsProfileMode::SecureRemote:
        return trEnum("Secure Remote DNS");
    case DnsProfileMode::ChinaDirectGlobalRemote:
        return trEnum("China Direct / Global Remote");
    case DnsProfileMode::Custom:
        return trEnum("Custom");
    }
    return trEnum("Custom");
}

QString TranslatableEnums::trDnsQueryStrategy(DnsQueryStrategy strategy)
{
    switch (strategy) {
    case DnsQueryStrategy::UseSystemDefault:
        return trEnum("Use system default");
    case DnsQueryStrategy::UseIP:
        return trEnum("Use IP");
    case DnsQueryStrategy::UseIPv4:
        return trEnum("Use IPv4");
    case DnsQueryStrategy::UseIPv6:
        return trEnum("Use IPv6");
    }
    return trEnum("Use system default");
}

QString TranslatableEnums::trGeoDataStatus(GeoDataStatus status)
{
    switch (status) {
    case GeoDataStatus::Missing:
        return trEnum("Missing");
    case GeoDataStatus::Present:
        return trEnum("Present");
    case GeoDataStatus::Updating:
        return trEnum("Updating");
    case GeoDataStatus::Verified:
        return trEnum("Verified");
    case GeoDataStatus::ChecksumFailed:
        return trEnum("Checksum failed");
    case GeoDataStatus::DownloadFailed:
        return trEnum("Download failed");
    case GeoDataStatus::NotWritable:
        return trEnum("Not writable");
    case GeoDataStatus::Unknown:
        return trEnum("Unknown");
    }
    return trEnum("Unknown");
}

QString TranslatableEnums::trRuleSetStatus(RuleSetStatus status)
{
    switch (status) {
    case RuleSetStatus::Missing:
        return trEnum("Missing");
    case RuleSetStatus::Present:
        return trEnum("Present");
    case RuleSetStatus::Updating:
        return trEnum("Updating");
    case RuleSetStatus::Verified:
        return trEnum("Verified");
    case RuleSetStatus::ChecksumFailed:
        return trEnum("Checksum failed");
    case RuleSetStatus::CompileFailed:
        return trEnum("Compile failed");
    case RuleSetStatus::DownloadFailed:
        return trEnum("Download failed");
    case RuleSetStatus::Unsupported:
        return trEnum("Unsupported");
    case RuleSetStatus::SourceMissing:
        return trEnum("Source missing");
    case RuleSetStatus::Unknown:
        return trEnum("Unknown");
    }
    return trEnum("Unknown");
}

QString TranslatableEnums::trKillSwitchStatus(KillSwitchStatus status)
{
    switch (status) {
    case KillSwitchStatus::Disabled:
        return trEnum("Disabled");
    case KillSwitchStatus::Enabling:
        return trEnum("Enabling");
    case KillSwitchStatus::Enabled:
        return trEnum("Enabled");
    case KillSwitchStatus::Disabling:
        return trEnum("Disabling");
    case KillSwitchStatus::Failed:
        return trEnum("Failed");
    case KillSwitchStatus::NeedsRecovery:
        return trEnum("Needs recovery");
    case KillSwitchStatus::Unsupported:
        return trEnum("Unsupported");
    }
    return trEnum("Unknown");
}

QString TranslatableEnums::trErrorSeverity(ErrorSeverity severity)
{
    switch (severity) {
    case ErrorSeverity::Info:
        return trEnum("Info");
    case ErrorSeverity::Warning:
        return trEnum("Warning");
    case ErrorSeverity::Error:
        return trEnum("Error");
    case ErrorSeverity::Critical:
        return trEnum("Critical");
    }
    return trEnum("Error");
}

} // namespace zarya
