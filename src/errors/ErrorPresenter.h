#pragma once

#include "errors/AppError.h"

class QWidget;

namespace zarya {

class ErrorPresenter {
public:
    static void show(QWidget* parent, const AppError& error, bool includeCopyDiagnostics = true);
};

} // namespace zarya
