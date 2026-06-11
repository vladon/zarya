#pragma once

#include <QString>

class QCoreApplication;

namespace zarya {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

struct StartupOptions {
    bool portable = false;
    bool minimized = false;
    bool noAutostartProfile = false;
    bool postUpdateNotice = false;
    bool updateRollbackNotice = false;
    bool printVersionAndExit = false;
    QString startProfileId;
    LogLevel logLevel = LogLevel::Info;

    bool startMinimizedEffective(bool settingStartMinimizedToTray) const;
};

class StartupOptionsParser {
public:
    static StartupOptions parse(QCoreApplication& app);
    static QString logLevelToString(LogLevel level);
};

} // namespace zarya
