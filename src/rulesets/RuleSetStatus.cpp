#include "rulesets/RuleSetStatus.h"

#include "i18n/TranslatableEnums.h"

namespace zarya {

QString ruleSetStatusToString(RuleSetStatus status)
{
    switch (status) {
    case RuleSetStatus::Missing:
        return QStringLiteral("missing");
    case RuleSetStatus::Present:
        return QStringLiteral("present");
    case RuleSetStatus::Updating:
        return QStringLiteral("updating");
    case RuleSetStatus::Verified:
        return QStringLiteral("verified");
    case RuleSetStatus::ChecksumFailed:
        return QStringLiteral("checksum-failed");
    case RuleSetStatus::CompileFailed:
        return QStringLiteral("compile-failed");
    case RuleSetStatus::DownloadFailed:
        return QStringLiteral("download-failed");
    case RuleSetStatus::Unsupported:
        return QStringLiteral("unsupported");
    case RuleSetStatus::SourceMissing:
        return QStringLiteral("source-missing");
    case RuleSetStatus::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

QString ruleSetStatusDisplayName(RuleSetStatus status)
{
    return TranslatableEnums::trRuleSetStatus(status);
}

} // namespace zarya
