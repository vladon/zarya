#include "updater/runner/UpdaterApplication.h"

#include "app/BuildInfo.h"
#include "storage/AppPaths.h"
#include "updater/AppUpdatePaths.h"
#include "updater/runner/PortableUpdateApplier.h"
#include "updater/runner/UpdatePlanFile.h"
#include "updater/runner/UpdaterLog.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QTextStream>

namespace zarya {

UpdaterApplication::UpdaterApplication(int& argc, char** argv)
    : m_argc(argc)
    , m_argv(argv)
{
}

int UpdaterApplication::printVersion() const
{
    QTextStream(stdout) << BuildInfo::updaterCliVersionText() << '\n';
    return 0;
}

int UpdaterApplication::printHelp() const
{
    QTextStream(stdout)
        << QStringLiteral("Zarya portable app updater\n\n")
        << QStringLiteral("Usage:\n")
        << QStringLiteral("  zarya-updater --plan <path>\n")
        << QStringLiteral("  zarya-updater --version\n")
        << QStringLiteral("  zarya-updater --help\n");
    return 0;
}

int UpdaterApplication::runPlan(const QString& planPath)
{
    UpdatePlan plan;
    QString error;
    if (!UpdatePlan::readFile(planPath, &plan, &error)) {
        QTextStream(stderr) << error << '\n';
        return 1;
    }

    AppUpdatePaths::ensureDirectories();
    AppPaths::initialize(true);

    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString logPath =
        QDir(AppUpdatePaths::logsDir()).filePath(QStringLiteral("update-%1.log").arg(timestamp));
    UpdaterLog log(logPath);
    PortableUpdateApplier applier(log);

    if (!applier.apply(plan, &error)) {
        log.error(error);
        QTextStream(stderr) << error << '\n';
        return 1;
    }
    return 0;
}

int UpdaterApplication::run()
{
    QCoreApplication app(m_argc, m_argv);
    QCoreApplication::setApplicationName(QStringLiteral("zarya-updater"));
    QCoreApplication::setApplicationVersion(BuildInfo::appVersion());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Zarya portable app updater"));
    parser.addHelpOption();
    QCommandLineOption versionOption({QStringLiteral("V"), QStringLiteral("version")},
                                     QStringLiteral("Display version information"));
    QCommandLineOption planOption(QStringLiteral("plan"),
                                  QStringLiteral("Apply update from plan file"),
                                  QStringLiteral("path"));
    parser.addOption(versionOption);
    parser.addOption(planOption);
    parser.process(app);

    if (parser.isSet(versionOption)) {
        return printVersion();
    }
    if (parser.isSet(planOption)) {
        return runPlan(parser.value(planOption));
    }
    if (parser.isSet(QStringLiteral("help"))) {
        return printHelp();
    }

    QTextStream(stderr) << QStringLiteral("Missing --plan argument.\n");
    return 2;
}

} // namespace zarya
