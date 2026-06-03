#include "platform/macos/MacAutostartManager.h"

#include "platform/PlatformProcessUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <unistd.h>

namespace zarya {

namespace {

QString xmlEscape(const QString& text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    escaped.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    escaped.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    return escaped;
}

} // namespace

QString MacAutostartManager::label()
{
    return QStringLiteral("dev.vladon.zarya");
}

QString MacAutostartManager::plistPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
        .filePath(QStringLiteral("Library/LaunchAgents/%1.plist").arg(label()));
}

QString MacAutostartManager::executablePath()
{
    return QCoreApplication::applicationFilePath();
}

bool MacAutostartManager::isSupported() const
{
    return true;
}

QString MacAutostartManager::backendName() const
{
    return QStringLiteral("macOS LaunchAgent");
}

QString MacAutostartManager::limitations() const
{
    return QStringLiteral(
        "Uses a user LaunchAgent plist. Unsigned builds may require manual approval. "
        "SMLoginItem is not used in this milestone.");
}

bool MacAutostartManager::isEnabled(QString* errorMessage) const
{
    Q_UNUSED(errorMessage);
    return QFile::exists(plistPath());
}

bool MacAutostartManager::writePlist(const QStringList& arguments, QString* errorMessage)
{
    QStringList programArguments;
    programArguments << executablePath();
    programArguments.append(arguments);

    QString plist;
    plist += QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    plist += QStringLiteral("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                            "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
    plist += QStringLiteral("<plist version=\"1.0\">\n<dict>\n");
    plist += QStringLiteral("  <key>Label</key>\n  <string>%1</string>\n").arg(label());
    plist += QStringLiteral("  <key>ProgramArguments</key>\n  <array>\n");
    for (const QString& argument : programArguments) {
        plist += QStringLiteral("    <string>%1</string>\n").arg(xmlEscape(argument));
    }
    plist += QStringLiteral("  </array>\n");
    plist += QStringLiteral("  <key>RunAtLoad</key>\n  <true/>\n");
    plist += QStringLiteral("  <key>KeepAlive</key>\n  <false/>\n");
    plist += QStringLiteral("</dict>\n</plist>\n");

    QFileInfo info(plistPath());
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QFile file(plistPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(plist.toUtf8());
    return true;
}

bool MacAutostartManager::enable(const QStringList& arguments, QString* errorMessage)
{
    if (!writePlist(arguments, errorMessage)) {
        return false;
    }

    const ProcessResult bootstrap = runProcess(
        QStringLiteral("launchctl"),
        {QStringLiteral("bootstrap"), QStringLiteral("gui/%1").arg(getuid()), plistPath()});
    if (!bootstrap.success && errorMessage) {
        *errorMessage =
            QStringLiteral("LaunchAgent plist written, but launchctl bootstrap failed: %1")
                .arg(bootstrap.errorMessage);
    }
    return true;
}

bool MacAutostartManager::disable(QString* errorMessage)
{
    const QString path = plistPath();
    if (QFile::exists(path)) {
        const ProcessResult bootout =
            runProcess(QStringLiteral("launchctl"),
                       {QStringLiteral("bootout"), QStringLiteral("gui/%1").arg(getuid()), path});
        Q_UNUSED(bootout);
        if (!QFile::remove(path)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Failed to remove LaunchAgent plist.");
            }
            return false;
        }
    }
    return true;
}

} // namespace zarya
