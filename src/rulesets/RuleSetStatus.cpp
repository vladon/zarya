#include "rulesets/RuleSetStatus.h"

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
    switch (status) {
    case RuleSetStatus::Missing:
        return QStringLiteral("Missing");
    case RuleSetStatus::Present:
        return QStringLiteral("Present");
    case RuleSetStatus::Updating:
        return QStringLiteral("Updating");
    case RuleSetStatus::Verified:
        return QStringLiteral("Verified");
    case RuleSetStatus::ChecksumFailed:
        return QStringLiteral("Checksum failed");
    case RuleSetStatus::CompileFailed:
        return QStringLiteral("Compile failed");
    case RuleSetStatus::DownloadFailed:
        return QStringLiteral("Download failed");
    case RuleSetStatus::Unsupported:
        return QStringLiteral("Unsupported");
    case RuleSetStatus::SourceMissing:
        return QStringLiteral("Source missing");
    case RuleSetStatus::Unknown:
        break;
    }
    return QStringLiteral("Unknown");
}

} // namespace zarya
