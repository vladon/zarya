#include "app/Application.h"
#include "app/StartupOptions.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    zarya::Application app(argc, argv);
    const zarya::StartupOptions& options = app.startupOptions();

    zarya::MainWindow window;
    window.logStartupContext(options);
    window.finishStartup(options);
    return app.exec();
}
