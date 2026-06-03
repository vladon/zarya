#include "app/Application.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    zarya::Application app(argc, argv);
    zarya::MainWindow window;
    window.show();
    return app.exec();
}
