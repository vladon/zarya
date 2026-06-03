#include "platform/AutostartManagerFactory.h"

#ifdef Q_OS_WIN
#include "platform/windows/WindowsAutostartManager.h"
#elif defined(Q_OS_MACOS)
#include "platform/macos/MacAutostartManager.h"
#elif defined(Q_OS_LINUX)
#include "platform/linux/LinuxAutostartManager.h"
#else
#include "platform/stub/StubAutostartManager.h"
#endif

namespace zarya {

std::unique_ptr<IAutostartManager> AutostartManagerFactory::create()
{
#ifdef Q_OS_WIN
    return std::make_unique<WindowsAutostartManager>();
#elif defined(Q_OS_MACOS)
    return std::make_unique<MacAutostartManager>();
#elif defined(Q_OS_LINUX)
    return std::make_unique<LinuxAutostartManager>();
#else
    return std::make_unique<StubAutostartManager>();
#endif
}

} // namespace zarya
