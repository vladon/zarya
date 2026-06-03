#include "platform/PlatformProcessUtils.h"

#include <QProcess>

namespace zarya {

ProcessResult runProcess(const QString& program, const QStringList& arguments, int timeoutMs)
{
    ProcessResult result;
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();

    if (!process.waitForStarted(timeoutMs)) {
        result.errorMessage =
            QStringLiteral("Failed to start %1: %2").arg(program, process.errorString());
        return result;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        result.errorMessage = QStringLiteral("%1 timed out after %2 ms").arg(program).arg(timeoutMs);
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    result.success = result.exitCode == 0;
    if (!result.success && result.errorMessage.isEmpty()) {
        result.errorMessage = result.standardError.isEmpty() ? result.standardOutput
                                                             : result.standardError;
    }
    return result;
}

} // namespace zarya
