#pragma once

#include <QStringList>

namespace zarya {

struct SettingsValidationResult {
    bool ok = true;
    QStringList autoFixed;
    QStringList warnings;
    QStringList errors;
};

class SettingsValidator {
public:
    static SettingsValidationResult validateAndFixOnStartup();
};

} // namespace zarya
