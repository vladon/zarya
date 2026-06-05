#include "rulesets/RuleSetCompiler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace zarya {

RuleSetCompileResult RuleSetCompiler::compileJsonToSrs(const QString& singBoxPath,
                                                        const QString& sourceJsonPath,
                                                        const QString& outputSrsPath,
                                                        int timeoutMs) const
{
    RuleSetCompileResult result;
    if (!QFile::exists(singBoxPath)) {
        result.errorMessage =
            QStringLiteral("sing-box executable not found: %1").arg(singBoxPath);
        return result;
    }
    if (!QFile::exists(sourceJsonPath)) {
        result.errorMessage =
            QStringLiteral("Source JSON not found: %1").arg(sourceJsonPath);
        return result;
    }

    QDir().mkpath(QFileInfo(outputSrsPath).absolutePath());
    const QString tempPath = outputSrsPath + QStringLiteral(".tmp");

    QProcess process;
    process.setProgram(singBoxPath);
    process.setArguments({QStringLiteral("rule-set"), QStringLiteral("compile"),
                          QStringLiteral("--output"), tempPath, sourceJsonPath});
    process.start();
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        result.errorMessage = QStringLiteral("sing-box rule-set compile timed out.");
        QFile::remove(tempPath);
        return result;
    }

    result.output = QString::fromUtf8(process.readAllStandardOutput());
    const QString stderrText = QString::fromUtf8(process.readAllStandardError());
    if (!stderrText.trimmed().isEmpty()) {
        result.output += stderrText;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.errorMessage = stderrText.trimmed().isEmpty()
                                  ? QStringLiteral("sing-box rule-set compile failed.")
                                  : stderrText.trimmed();
        QFile::remove(tempPath);
        return result;
    }
    if (!QFile::exists(tempPath)) {
        result.errorMessage = QStringLiteral("Compile succeeded but output file is missing.");
        return result;
    }

    if (QFile::exists(outputSrsPath)) {
        QFile::remove(outputSrsPath + QStringLiteral(".bak"));
        QFile::rename(outputSrsPath, outputSrsPath + QStringLiteral(".bak"));
    }
    if (!QFile::rename(tempPath, outputSrsPath)) {
        QFile::remove(outputSrsPath);
        if (!QFile::rename(tempPath, outputSrsPath)) {
            result.errorMessage = QStringLiteral("Could not replace output rule set file.");
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace zarya
