#include "app/StartupOptions.h"

#include <QCommandLineParser>
#include <QCoreApplication>

namespace zarya {

namespace {

LogLevel logLevelFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("debug")) {
        return LogLevel::Debug;
    }
    if (normalized == QStringLiteral("warn") || normalized == QStringLiteral("warning")) {
        return LogLevel::Warn;
    }
    if (normalized == QStringLiteral("error")) {
        return LogLevel::Error;
    }
    return LogLevel::Info;
}

} // namespace

bool StartupOptions::startMinimizedEffective(bool settingStartMinimizedToTray) const
{
    return minimized || settingStartMinimizedToTray;
}

StartupOptions StartupOptionsParser::parse(QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Zarya proxy client"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portableOption({QStringLiteral("p"), QStringLiteral("portable")},
                                      QStringLiteral("Store config in app-local data directory"));
    QCommandLineOption minimizedOption({QStringLiteral("m"), QStringLiteral("minimized")},
                                       QStringLiteral("Start hidden to system tray"));
    QCommandLineOption noAutostartProfileOption(
        QStringLiteral("no-autostart-profile"),
        QStringLiteral("Do not auto-start the last used profile"));
    QCommandLineOption startProfileOption(QStringLiteral("start-profile"),
                                          QStringLiteral("Start profile by ID after launch"),
                                          QStringLiteral("profileId"));
    QCommandLineOption logLevelOption(QStringLiteral("log-level"),
                                      QStringLiteral("Log verbosity: debug, info, warn, error"),
                                      QStringLiteral("level"), QStringLiteral("info"));

    parser.addOption(portableOption);
    parser.addOption(minimizedOption);
    parser.addOption(noAutostartProfileOption);
    parser.addOption(startProfileOption);
    parser.addOption(logLevelOption);
    parser.process(app);

    StartupOptions options;
    options.portable = parser.isSet(portableOption);
    options.minimized = parser.isSet(minimizedOption);
    options.noAutostartProfile = parser.isSet(noAutostartProfileOption);
    options.startProfileId = parser.value(startProfileOption);
    options.logLevel = logLevelFromString(parser.value(logLevelOption));
    return options;
}

QString StartupOptionsParser::logLevelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return QStringLiteral("debug");
    case LogLevel::Info:
        return QStringLiteral("info");
    case LogLevel::Warn:
        return QStringLiteral("warn");
    case LogLevel::Error:
        return QStringLiteral("error");
    }
    return QStringLiteral("info");
}

} // namespace zarya
