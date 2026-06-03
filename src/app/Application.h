#pragma once

#include "app/StartupOptions.h"

#include <QApplication>

namespace zarya {

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    static Application* instance();

    const StartupOptions& startupOptions() const;

private:
    StartupOptions m_startupOptions;
};

} // namespace zarya
