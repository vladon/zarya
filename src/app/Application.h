#pragma once

#include <QApplication>

namespace zarya {

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    static Application* instance();
};

} // namespace zarya
