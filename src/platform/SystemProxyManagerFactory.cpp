#include "platform/SystemProxyManagerFactory.h"

#ifdef Q_OS_WIN
#include "platform/windows/WindowsSystemProxyManager.h"
#else
#include "platform/stub/StubSystemProxyManager.h"
#endif

namespace zarya {

std::unique_ptr<ISystemProxyManager> SystemProxyManagerFactory::create()
{
#ifdef Q_OS_WIN
    return std::make_unique<WindowsSystemProxyManager>();
#else
    return std::make_unique<StubSystemProxyManager>();
#endif
}

} // namespace zarya
