#pragma once

#include "migration/MigrationResult.h"

namespace zarya {

class MigrationManager {
public:
    static MigrationResult runStartupMigrations();
    static const MigrationResult& lastResult();
};

} // namespace zarya
