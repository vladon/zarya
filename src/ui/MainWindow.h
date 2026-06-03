#pragma once

#include <QMainWindow>

namespace zarya {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
};

} // namespace zarya
