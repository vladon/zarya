#pragma once

#include <QString>

namespace zarya {

enum class RuleSetStatus {
    Missing,
    Present,
    Updating,
    Verified,
    ChecksumFailed,
    CompileFailed,
    DownloadFailed,
    Unsupported,
    SourceMissing,
    Unknown,
};

QString ruleSetStatusToString(RuleSetStatus status);
QString ruleSetStatusDisplayName(RuleSetStatus status);

} // namespace zarya
