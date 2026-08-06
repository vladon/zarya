#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

struct ManagedCoreOrphanCleanupResult {
    int terminatedCount = 0;
    QStringList details;
};

// Terminates leftover sing-box and core-test-worker processes that match app-owned
// paths. Embedded Xray never has a process to clean up.
ManagedCoreOrphanCleanupResult terminateOrphanedManagedCores();

// True if any process currently matches a managed core executable path.
bool hasOrphanedManagedCores();

} // namespace zarya
