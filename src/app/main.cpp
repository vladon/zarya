#include "app/Application.h"
#include "app/BuildInfo.h"
#include "app/StartupOptions.h"
#include "storage/AppSettings.h"
#include "ui/MainWindow.h"

#include <QCoreApplication>
#include <QTextStream>

#include <string_view>

namespace {

bool versionRequested(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--version" || argument == "-V") {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[])
{
    // Keep release verification usable from headless build agents. On macOS,
    // constructing QApplication first asks LaunchServices to register a GUI
    // process and may abort before Qt can parse --version.
    if (versionRequested(argc, argv)) {
        QCoreApplication app(argc, argv);
        QTextStream(stdout) << zarya::BuildInfo::cliVersionText() << '\n';
        return 0;
    }

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
