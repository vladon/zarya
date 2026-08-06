#include "errors/ErrorPresenter.h"

#include "errors/ErrorCode.h"
#include "i18n/ZaryaTr.h"
#include "ui/desktopapp/UiMessagePresenter.h"

#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QVector>

namespace zarya {

namespace {

QString actionId(ErrorAction action)
{
    return QString::number(static_cast<int>(action));
}

void addAction(
    QVector<UiMessageAction>* actions,
    QHash<QString, ErrorAction>* actionMap,
    const QString& text,
    ErrorAction action,
    UiMessageActionRole role = UiMessageActionRole::Secondary,
    bool isDefault = false,
    bool isCancel = false)
{
    const QString id = actionId(action);
    actions->push_back({id, text, role, isDefault, isCancel});
    actionMap->insert(id, action);
}

} // namespace

void ErrorPresenter::show(QWidget* parent, const AppError& error, bool includeCopyDiagnostics)
{
    showWithActions(parent, error, includeCopyDiagnostics);
}

ErrorAction ErrorPresenter::showWithActions(QWidget* parent, const AppError& error,
                                            bool includeCopyDiagnostics)
{
    QString text = error.message;
    if (!error.suggestedAction.isEmpty()) {
        text += QStringLiteral("\n\n") + error.suggestedAction;
    }
    if (!error.details.isEmpty()) {
        text += QStringLiteral("\n\nCode: %1\nArea: %2\n\n%3")
                    .arg(error.code, error.area, error.details);
    }

    QVector<UiMessageAction> actions;
    QHash<QString, ErrorAction> actionMap;
    if (error.code == ErrorCode::coreXrayMissing()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Create Diagnostics"),
                  ErrorAction::CreateDiagnostics, UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::coreValidationFailed()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Show Details"), ErrorAction::ShowDetails,
                  UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Create Diagnostics"),
                  ErrorAction::CreateDiagnostics);
        addAction(&actions, &actionMap, ZaryaTr::tr("OK"), ErrorAction::Ok,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::systemProxyRestoreFailed()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Retry Restore"), ErrorAction::RetryRestore,
                  UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Open Settings"), ErrorAction::OpenSettings);
        addAction(&actions, &actionMap, ZaryaTr::tr("Create Diagnostics"),
                  ErrorAction::CreateDiagnostics);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::killSwitchNeedsRecovery()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Recover"), ErrorAction::RecoverKillSwitch,
                  UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Show Recovery Instructions"),
                  ErrorAction::ShowRecoveryInstructions);
        addAction(&actions, &actionMap, ZaryaTr::tr("Create Diagnostics"),
                  ErrorAction::CreateDiagnostics);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::ruleSetMissing()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Open Rule Set Manager"),
                  ErrorAction::OpenRuleSetManager, UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Continue"), ErrorAction::Continue);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::geoDataMissing()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Open Geo Data Manager"),
                  ErrorAction::OpenGeoDataManager, UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Continue"), ErrorAction::Continue);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else if (error.code == ErrorCode::profileUnsupportedRuntime()) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Switch to Xray system proxy"),
                  ErrorAction::SwitchToSystemProxy, UiMessageActionRole::Primary, true);
        addAction(&actions, &actionMap, ZaryaTr::tr("Open Profile"), ErrorAction::OpenProfile);
        addAction(&actions, &actionMap, ZaryaTr::tr("Cancel"), ErrorAction::Cancel,
                  UiMessageActionRole::Secondary, false, true);
    } else {
        addAction(&actions, &actionMap, ZaryaTr::tr("OK"), ErrorAction::Ok,
                  UiMessageActionRole::Primary, true, true);
    }

    if (includeCopyDiagnostics) {
        addAction(&actions, &actionMap, ZaryaTr::tr("Copy details"),
                  ErrorAction::CopyDetails);
    }

    const QString selectedId = UiMessagePresenter::choose(
        parent,
        error.title.isEmpty() ? ZaryaTr::tr("Zarya") : error.title,
        text,
        error.severity == ErrorSeverity::Critical ? UiMessageTone::Error
                                                  : UiMessageTone::Warning,
        actions);
    const ErrorAction selected = actionMap.value(selectedId, ErrorAction::None);
    if (selected == ErrorAction::CopyDetails) {
        const QString clipboard = QStringLiteral("[%1] %2\n%3\n%4")
                                      .arg(error.code, error.title, error.message, error.details);
        QApplication::clipboard()->setText(clipboard);
    }
    return selected;
}

} // namespace zarya
