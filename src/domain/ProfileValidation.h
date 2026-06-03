#pragma once

#include "domain/Profile.h"

#include <QString>

namespace zarya {

struct ProfileValidationResult {
    bool ok = false;
    QString message;
};

ProfileValidationResult validateProfileForDialog(const Profile& profile);
ProfileValidationResult validateProfileForXray(const Profile& profile);

} // namespace zarya
