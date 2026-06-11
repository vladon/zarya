#include "helper/HelperApplication.h"

#if defined(Q_OS_WIN)
#include "helper/WindowsServiceRunner.h"
#endif

int main(int argc, char* argv[])
{
#if defined(Q_OS_WIN)
    if (zarya::WindowsServiceRunner::shouldRunAsService(argc, argv)) {
        const int serviceCode = zarya::WindowsServiceRunner::runAsService(argc, argv);
        if (serviceCode >= 0) {
            return serviceCode;
        }
    }
#endif

    zarya::HelperApplication application(argc, argv);
    return application.run();
}
