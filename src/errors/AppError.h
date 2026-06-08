#pragma once

#include <QString>

namespace zarya {

enum class ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical,
};

struct AppError {
    QString code;
    QString title;
    QString message;
    QString details;
    QString suggestedAction;
    ErrorSeverity severity = ErrorSeverity::Error;
    QString area;
};

QString errorSeverityToString(ErrorSeverity severity);
AppError appErrorFromCode(const QString& code, const QString& details = {});

} // namespace zarya
