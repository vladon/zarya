#include "service/HelperServiceManagerFactory.h"

#include "service/IHelperServiceManager.h"

#if defined(Q_OS_WIN)
#include "service/windows/WindowsHelperServiceManager.h"
#elif defined(Q_OS_MACOS)
#include "service/macos/MacHelperServiceManager.h"
#elif defined(Q_OS_LINUX)
#include "service/linux/LinuxHelperServiceManager.h"
#else
#include "service/stub/StubHelperServiceManager.h"
#endif

namespace zarya {

std::unique_ptr<IHelperServiceManager> HelperServiceManagerFactory::create()
{
#if defined(Q_OS_WIN)
    return std::make_unique<WindowsHelperServiceManager>();
#elif defined(Q_OS_MACOS)
    return std::make_unique<MacHelperServiceManager>();
#elif defined(Q_OS_LINUX)
    return std::make_unique<LinuxHelperServiceManager>();
#else
    return std::make_unique<StubHelperServiceManager>();
#endif
}

} // namespace zarya
