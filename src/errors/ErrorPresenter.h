#pragma once

#include "errors/AppError.h"

#include <QString>

class QWidget;

namespace zarya {

enum class ErrorAction {
    None,
    Ok,
    CopyDetails,
    OpenCoreManager,
    ChooseExistingBinary,
    ShowDetails,
    CreateDiagnostics,
    RetryRestore,
    OpenSettings,
    RecoverKillSwitch,
    ShowRecoveryInstructions,
    OpenRuleSetManager,
    Continue,
    Cancel,
    OpenGeoDataManager,
    SwitchToSystemProxy,
    OpenProfile,
};

class ErrorPresenter {
public:
    static void show(QWidget* parent, const AppError& error, bool includeCopyDiagnostics = true);
    static ErrorAction showWithActions(QWidget* parent, const AppError& error,
                                       bool includeCopyDiagnostics = true);
};

} // namespace zarya
