#include "ui/MainWindow.h"

namespace zarya {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Zarya"));
    resize(960, 640);
    statusBar()->showMessage(QStringLiteral("Ready"));
}

} // namespace zarya
