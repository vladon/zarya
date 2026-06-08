#include "cores/CoreVersionDetector.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>

namespace zarya {

QString CoreVersionDetector::normalizeVersion(QString version)
{
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        version = version.mid(1);
    }
    return version;
}

DetectedVersion CoreVersionDetector::detect(const QString& executablePath, CoreType type,
                                            int timeoutMs)
{
    DetectedVersion result;
    if (executablePath.trimmed().isEmpty() || !QFile::exists(executablePath)) {
        result.error = QStringLiteral("Core executable not found.");
        return result;
    }

    QProcess process;
    process.setProgram(executablePath);
    process.setArguments({QStringLiteral("version")});
    process.start();
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        result.error = QStringLiteral("Version command timed out.");
        return result;
    }

    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    const QString stderrText = QString::fromUtf8(process.readAllStandardError());
    result.rawOutput = stdoutText.trimmed();
    if (result.rawOutput.isEmpty()) {
        result.rawOutput = stderrText.trimmed();
    }

    if (process.exitStatus() != QProcess::NormalExit) {
        result.error = QStringLiteral("Version command failed.");
        return result;
    }

    static const QRegularExpression versionPattern(
        QStringLiteral(R"((\d+\.\d+\.\d+(?:[-+.\w]*)?))"));
    const QRegularExpressionMatch match = versionPattern.match(result.rawOutput);
    if (match.hasMatch()) {
        result.version = normalizeVersion(match.captured(1));
        result.ok = !result.version.isEmpty();
        return result;
    }

    if (type == CoreType::Xray && result.rawOutput.contains(QStringLiteral("Xray"), Qt::CaseInsensitive)) {
        result.ok = true;
        result.version = result.rawOutput;
        return result;
    }

    result.error = QStringLiteral("Could not parse version from output.");
    return result;
}

} // namespace zarya
