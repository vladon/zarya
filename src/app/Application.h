#pragma once

#include "app/StartupOptions.h"

#include <QApplication>

class QEvent;
class QObject;

namespace zarya {

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    static Application* instance();

    const StartupOptions& startupOptions() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    StartupOptions m_startupOptions;
};

} // namespace zarya
