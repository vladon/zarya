#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct ManagedCoreOrphanCleanupResult {
    int terminatedCount = 0;
    QStringList details;
};

// Terminates leftover xray/sing-box processes that match Zarya-managed executable
// paths (resolved settings paths). Safe to call when CoreManager is not running.
ManagedCoreOrphanCleanupResult terminateOrphanedManagedCores();

// True if any process currently matches a managed core executable path.
bool hasOrphanedManagedCores();

} // namespace zarya
