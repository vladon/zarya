#include "ipc/IpcTransport.h"

#include <QProcessEnvironment>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace zarya {

QString IpcTransport::defaultServerName()
{
#if defined(Q_OS_WIN)
    const QString user = qEnvironmentVariable("USERNAME");
    if (!user.isEmpty()) {
        return QStringLiteral("zarya-helper-%1").arg(user);
    }
    return QStringLiteral("zarya-helper-default");
#else
    return QStringLiteral("zarya-helper-%1").arg(static_cast<qulonglong>(getuid()));
#endif
}

QString IpcTransport::serviceServerName(const QString& serviceName)
{
    return QStringLiteral("zarya-helper-service-%1").arg(serviceName);
}

} // namespace zarya
