#pragma once

#include "helper/HelperPathPolicy.h"

#include <QDateTime>
#include <QObject>
#include <QProcess>

namespace zarya {

class HelperRuntimeManager : public QObject {
    Q_OBJECT

public:
    explicit HelperRuntimeManager(QObject* parent = nullptr);

    void setPathPolicy(const HelperPathPolicy* policy);

    bool isRunning() const;
    qint64 processId() const;
    QString configPath() const;
    QDateTime startedAt() const;

    bool validateConfig(const QString& singBoxPath, const QString& configPath,
                        QString* output = nullptr, QString* errorMessage = nullptr);
    bool startTun(const QString& singBoxPath, const QString& configPath,
                  const QString& workingDirectory, bool checkBeforeStart,
                  QString* errorMessage = nullptr);
    bool stopTun(QString* errorMessage = nullptr);

signals:
    void logLine(const QString& line);
    void runtimeExited(int exitCode);

private:
    QString runProcess(const QString& executable, const QStringList& arguments, int timeoutMs,
                       int* exitCode) const;

    const HelperPathPolicy* m_pathPolicy = nullptr;
    QProcess m_process;
    QString m_configPath;
    QDateTime m_startedAt;
};

} // namespace zarya
