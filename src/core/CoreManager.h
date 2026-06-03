#pragma once

#include <QObject>
#include <QProcess>
#include <memory>

namespace zarya {

class CoreManager : public QObject {
    Q_OBJECT

public:
    explicit CoreManager(QObject* parent = nullptr);

    bool isRunning() const;
    QString runningCoreName() const;

    void start(const QString& coreExecutablePath, const QString& coreDisplayName,
               const QStringList& arguments);
    void stop();

signals:
    void started(const QString& coreName);
    void stopped();
    void logLine(const QString& line);
    void errorOccurred(const QString& message);

private:
    void appendProcessOutput(const QByteArray& data, bool isStdErr);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    QProcess m_process;
    QString m_runningCoreName;
    static constexpr int kStopTimeoutMs = 3000;
};

} // namespace zarya
