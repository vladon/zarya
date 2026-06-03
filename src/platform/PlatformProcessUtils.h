#pragma once

#include <QString>
#include <QStringList>

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

} // namespace zarya
