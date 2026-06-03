#include "platform/SystemProxyManagerFactory.h"

#ifdef Q_OS_WIN
#include "platform/windows/WindowsSystemProxyManager.h"
#elif defined(Q_OS_MACOS)
#include "platform/macos/MacSystemProxyManager.h"
#elif defined(Q_OS_LINUX)
#include "platform/linux/LinuxSystemProxyManager.h"
#else
#include "platform/stub/StubSystemProxyManager.h"
#endif

namespace zarya {

std::unique_ptr<ISystemProxyManager> SystemProxyManagerFactory::create()
{
#ifdef Q_OS_WIN
    return std::make_unique<WindowsSystemProxyManager>();
#elif defined(Q_OS_MACOS)
    return std::make_unique<MacSystemProxyManager>();
#elif defined(Q_OS_LINUX)
    return std::make_unique<LinuxSystemProxyManager>();
#else
    return std::make_unique<StubSystemProxyManager>();
#endif
}

} // namespace zarya
