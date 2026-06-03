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
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    if (!m_runningCoreName.isEmpty()) {
        emit logLine(QStringLiteral("Core process exited."));
    }
    m_runningCoreName.clear();
    emit stopped();
}

} // namespace zarya
