#include "helper/HelperRuntimeManager.h"

#include <QProcess>

namespace zarya {

HelperRuntimeManager::HelperRuntimeManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit logLine(QString::fromUtf8(m_process.readAllStandardOutput()).trimmed());
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        emit logLine(QString::fromUtf8(m_process.readAllStandardError()).trimmed());
    });
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus status) {
                Q_UNUSED(status);
                m_configPath.clear();
                emit runtimeExited(exitCode);
            });
}

void HelperRuntimeManager::setPathPolicy(const HelperPathPolicy* policy)
{
    m_pathPolicy = policy;
}

bool HelperRuntimeManager::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

qint64 HelperRuntimeManager::processId() const
{
    return m_process.processId();
}

QString HelperRuntimeManager::configPath() const
{
    return m_configPath;
}

QDateTime HelperRuntimeManager::startedAt() const
{
    return m_startedAt;
}

QString HelperRuntimeManager::runProcess(const QString& executable, const QStringList& arguments,
                                         int timeoutMs, int* exitCode) const
{
    QProcess process;
    process.setProgram(executable);
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
    return QString::fromUtf8(process.readAllStandardOutput() + process.readAllStandardError())
        .trimmed();
}

bool HelperRuntimeManager::validateConfig(const QString& singBoxPath, const QString& configPath,
                                          QString* output, QString* errorMessage)
{
    if (m_pathPolicy) {
        QString reason;
        if (!m_pathPolicy->isAllowedSingBoxPath(singBoxPath, &reason)
            || !m_pathPolicy->isAllowedConfigPath(configPath, &reason)) {
            if (errorMessage) {
                *errorMessage = reason;
            }
            return false;
        }
    }

    const QList<QStringList> argumentSets = {
        {QStringLiteral("check"), QStringLiteral("-c"), configPath},
        {QStringLiteral("run"), QStringLiteral("-test"), QStringLiteral("-c"), configPath},
    };

    for (const QStringList& arguments : argumentSets) {
        int exitCode = -1;
        const QString processOutput = runProcess(singBoxPath, arguments, 30000, &exitCode);
        if (output) {
            *output = processOutput;
        }
        if (exitCode == 0) {
            return true;
        }
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("sing-box config validation failed.");
    }
    return false;
}

bool HelperRuntimeManager::startTun(const QString& singBoxPath, const QString& configPath,
                                    const QString& workingDirectory, bool checkBeforeStart,
                                    QString* errorMessage)
{
    if (isRunning()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("sing-box is already running.");
        }
        return false;
    }

    if (m_pathPolicy) {
        QString reason;
        if (!m_pathPolicy->isAllowedSingBoxPath(singBoxPath, &reason)
            || !m_pathPolicy->isAllowedConfigPath(configPath, &reason)
            || !m_pathPolicy->isAllowedWorkingDirectory(workingDirectory, singBoxPath, &reason)) {
            if (errorMessage) {
                *errorMessage = reason;
            }
            return false;
        }
    }

    if (checkBeforeStart) {
        emit logLine(QStringLiteral("helper: running sing-box check"));
        if (!validateConfig(singBoxPath, configPath, nullptr, errorMessage)) {
            return false;
        }
        emit logLine(QStringLiteral("helper: validation OK"));
    }

    if (!workingDirectory.trimmed().isEmpty()) {
        m_process.setWorkingDirectory(workingDirectory);
    }

    m_process.setProgram(singBoxPath);
    m_process.setArguments({QStringLiteral("run"), QStringLiteral("-c"), configPath});
    emit logLine(QStringLiteral("helper: starting sing-box"));
    m_process.start();
    if (!m_process.waitForStarted(5000)) {
        if (errorMessage) {
            *errorMessage = m_process.errorString();
        }
        return false;
    }

    m_configPath = configPath;
    m_startedAt = QDateTime::currentDateTimeUtc();
    return true;
}

bool HelperRuntimeManager::stopTun(QString* errorMessage)
{
    if (!isRunning()) {
        return true;
    }

    emit logLine(QStringLiteral("helper: stopping sing-box"));
    m_process.terminate();
    if (!m_process.waitForFinished(5000)) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
    m_configPath.clear();
    emit logLine(QStringLiteral("helper: sing-box stopped"));
    Q_UNUSED(errorMessage);
    return true;
}

} // namespace zarya
