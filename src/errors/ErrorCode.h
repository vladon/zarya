#pragma once

#include <QString>

namespace zarya {

namespace ErrorCode {

inline QString coreXrayMissing() { return QStringLiteral("CORE_XRAY_MISSING"); }
inline QString coreValidationFailed() { return QStringLiteral("CORE_VALIDATION_FAILED"); }
inline QString systemProxyRestoreFailed() { return QStringLiteral("SYSTEM_PROXY_RESTORE_FAILED"); }
inline QString helperNotConnected() { return QStringLiteral("HELPER_NOT_CONNECTED"); }
inline QString killSwitchNeedsRecovery() { return QStringLiteral("KILLSWITCH_NEEDS_RECOVERY"); }
inline QString ruleSetMissing() { return QStringLiteral("RULESET_MISSING"); }
inline QString dnsProfileInvalid() { return QStringLiteral("DNS_PROFILE_INVALID"); }
inline QString routingProfileInvalid() { return QStringLiteral("ROUTING_PROFILE_INVALID"); }
inline QString backupImportChecksumFailed() { return QStringLiteral("BACKUP_IMPORT_CHECKSUM_FAILED"); }
inline QString migrationFailed() { return QStringLiteral("MIGRATION_FAILED"); }

} // namespace ErrorCode

} // namespace zarya
