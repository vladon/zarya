#pragma once

#include <QString>

namespace zarya {

class UpdaterApplication {
public:
    UpdaterApplication(int& argc, char** argv);

    int run();

private:
    int runPlan(const QString& planPath);
    int printHelp() const;
    int printVersion() const;

    int m_argc;
    char** m_argv;
};

} // namespace zarya
