#include "platform/PlatformPrivilege.h"

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace zarya {

PrivilegeCheckResult PlatformPrivilege::currentProcessPrivileges()
{
    PrivilegeCheckResult result;
#if defined(Q_OS_WIN)
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID administratorsGroup = nullptr;
    if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                   &administratorsGroup)) {
        CheckTokenMembership(nullptr, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    result.elevated = isAdmin == TRUE;
    result.summary = result.elevated ? QStringLiteral("elevated")
                                     : QStringLiteral("not elevated");
#elif defined(Q_OS_UNIX)
    result.elevated = geteuid() == 0;
    result.summary = result.elevated ? QStringLiteral("root") : QStringLiteral("not root");
#else
    result.summary = QStringLiteral("unknown");
#endif
    return result;
}

} // namespace zarya
