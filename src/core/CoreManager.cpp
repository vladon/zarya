#include "core/CoreManager.h"

namespace zarya {

CoreManager::CoreManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendProcessOutput(m_process.readAllStandardOutput(), false);
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendProcessOutput(m_process.readAllStandardError(), true);
    });
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &CoreManager::onProcessFinished);
}

bool CoreManager::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString CoreManager::runningCoreName() const
{
    return m_runningCoreName;
}

int CoreManager::lastExitCode() const
{
    return m_lastExitCode;
}

QString CoreManager::runProcess(const QString& coreExecutablePath,
                                const QStringList& arguments, int timeoutMs,
                                int* exitCode) const
{
    QProcess process;
    process.setProgram(coreExecutablePath);
    process.setArguments(arguments);
    process.start();

    if (!process.waitForStarted(5000)) {
        if (exitCode) {
            *exitCode = -1;
        }
        return process.errorString();
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        if (exitCode) {
            *exitCode = -1;
        }
        return QStringLiteral("Process timed out.");
    }

    if (exitCode) {
        *exitCode = process.exitCode();
    }

    const QByteArray stdoutBytes = process.readAllStandardOutput();
    const QByteArray stderrBytes = process.readAllStandardError();
    return QString::fromUtf8(stdoutBytes + stderrBytes).trimmed();
}

CoreValidationResult CoreManager::validateConfig(const QString& coreExecutablePath,
                                                 const QString& configPath) const
{
    CoreValidationResult result;
    const QStringList arguments =
        {QStringLiteral("run"), QStringLiteral("-test"), QStringLiteral("-config"), configPath};

    int exitCode = -1;
    result.output = runProcess(coreExecutablePath, arguments, 30000, &exitCode);
    result.exitCode = exitCode;
    result.success = exitCode == 0;

    if (!result.success) {
        result.errorMessage =
            exitCode < 0
                ? result.output
                : QStringLiteral("Xray config test failed (exit %1).").arg(exitCode);
    }
    return result;
}

void CoreManager::startCore(const QString& coreExecutablePath, const QString& configPath,
                            const QString& coreDisplayName)
{
    const QStringList arguments =
        {QStringLiteral("run"), QStringLiteral("-config"), configPath};
    start(coreExecutablePath, coreDisplayName, arguments);
}

void CoreManager::start(const QString& coreExecutablePath, const QString& coreDisplayName,
                        const QStringList& arguments)
{
    if (isRunning()) {
        emit errorOccurred(QStringLiteral("A core process is already running."));
        return;
    }

    m_runningCoreName = coreDisplayName;
    m_process.setProgram(coreExecutablePath);
    m_process.setArguments(arguments);

    emit logLine(QStringLiteral("Command: %1 %2")
                   .arg(coreExecutablePath, arguments.join(QLatin1Char(' '))));

    m_process.start();
    if (!m_process.waitForStarted(5000)) {
        const QString message =
            QStringLiteral("Failed to start %1: %2")
                .arg(coreDisplayName, m_process.errorString());
        m_runningCoreName.clear();
        emit errorOccurred(message);
        return;
    }

    emit started(coreDisplayName);
}

void CoreManager::stop()
{
    if (!isRunning()) {
        return;
    }

    m_process.terminate();
    if (!m_process.waitForFinished(kStopTimeoutMs)) {
        m_process.kill();
        m_process.waitForFinished(kStopTimeoutMs);
    }
}

void CoreManager::appendProcessOutput(const QByteArray& data, bool isStdErr)
{
    Q_UNUSED(isStdErr);
    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            emit logLine(trimmed);
        }
    }
}

void CoreManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);
    m_lastExitCode = exitCode;
    if (!m_runningCoreName.isEmpty()) {
        emit logLine(QStringLiteral("Core process exited (code %1).").arg(exitCode));
    }
    m_runningCoreName.clear();
    emit stopped();
}

} // namespace zarya
