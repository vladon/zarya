#include "app/Application.h"
#include "app/BuildInfo.h"
#include "app/StartupOptions.h"
#include "storage/AppSettings.h"
#include "ui/MainWindow.h"

#include <QTextStream>

int main(int argc, char* argv[])
{
    zarya::Application app(argc, argv);
    const zarya::StartupOptions& options = app.startupOptions();

    if (options.printVersionAndExit) {
        QTextStream(stdout) << zarya::BuildInfo::cliVersionText() << '\n';
        return 0;
    }

    zarya::MainWindow window;
    window.logStartupContext(options);
    window.finishStartup(options);
    return app.exec();
}
