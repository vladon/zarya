#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

namespace zarya {

struct CoreValidationResult {
    bool success = false;
    int exitCode = -1;
    QString output;
    QString errorMessage;
};

class CoreManager : public QObject {
    Q_OBJECT

public:
    explicit CoreManager(QObject* parent = nullptr);

    bool isRunning() const;
    QString runningCoreName() const;
    int lastExitCode() const;

    CoreValidationResult validateConfig(const QString& coreExecutablePath,
                                        const QString& configPath) const;
    void startCore(const QString& coreExecutablePath, const QString& configPath,
                   const QString& coreDisplayName);
    void stop();

    void start(const QString& coreExecutablePath, const QString& coreDisplayName,
               const QStringList& arguments);

signals:
    void started(const QString& coreName);
    void stopped();
    void logLine(const QString& line);
    void errorOccurred(const QString& message);

private:
    QString runProcess(const QString& coreExecutablePath, const QStringList& arguments,
                       int timeoutMs, int* exitCode) const;
    void appendProcessOutput(const QByteArray& data, bool isStdErr);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    QProcess m_process;
    QString m_runningCoreName;
    int m_lastExitCode = 0;
    static constexpr int kStopTimeoutMs = 3000;
};

} // namespace zarya
