#pragma once

#include <QString>
#include <QStringList>

#include <functional>

namespace zarya {

struct ProcessResult {
    bool success = false;
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
    QString errorMessage;
};

ProcessResult runProcess(const QString& program, const QStringList& arguments,
                         int timeoutMs = 5000);

using PlatformProcessRunner =
    std::function<ProcessResult(const QString&, const QStringList&, int timeoutMs)>;

PlatformProcessRunner defaultPlatformProcessRunner();

} // namespace zarya
