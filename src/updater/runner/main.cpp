#include "updater/runner/UpdaterApplication.h"

int main(int argc, char* argv[])
{
    zarya::UpdaterApplication application(argc, argv);
    return application.run();
}
