#include "errors/ErrorPresenter.h"

#include "errors/ErrorCode.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>

namespace zarya {

namespace {

void addActionButton(QMessageBox* box, const QString& text, ErrorAction action,
                     QHash<QAbstractButton*, ErrorAction>* actions)
{
    QPushButton* button = box->addButton(text, QMessageBox::ActionRole);
    actions->insert(button, action);
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

    QMessageBox box(parent);
    box.setIcon(error.severity == ErrorSeverity::Critical ? QMessageBox::Critical
                                                          : QMessageBox::Warning);
    box.setWindowTitle(error.title.isEmpty() ? QStringLiteral("Zarya") : error.title);
    box.setText(text);
    if (!error.details.isEmpty()) {
        box.setDetailedText(QStringLiteral("Code: %1\nArea: %2\n\n%3")
                                .arg(error.code, error.area, error.details));
    }

    QHash<QAbstractButton*, ErrorAction> actions;
    if (error.code == ErrorCode::coreXrayMissing()) {
        addActionButton(&box, QStringLiteral("Open Core Manager"), ErrorAction::OpenCoreManager,
                        &actions);
        addActionButton(&box, QStringLiteral("Choose Existing Binary"),
                        ErrorAction::ChooseExistingBinary, &actions);
        box.addButton(QMessageBox::Cancel);
    } else if (error.code == ErrorCode::coreValidationFailed()) {
        addActionButton(&box, QStringLiteral("Show Details"), ErrorAction::ShowDetails, &actions);
        addActionButton(&box, QStringLiteral("Create Diagnostics"), ErrorAction::CreateDiagnostics,
                        &actions);
        box.addButton(QMessageBox::Ok);
    } else if (error.code == ErrorCode::systemProxyRestoreFailed()) {
        addActionButton(&box, QStringLiteral("Retry Restore"), ErrorAction::RetryRestore, &actions);
        addActionButton(&box, QStringLiteral("Open Settings"), ErrorAction::OpenSettings, &actions);
        addActionButton(&box, QStringLiteral("Create Diagnostics"), ErrorAction::CreateDiagnostics,
                        &actions);
        box.addButton(QMessageBox::Cancel);
    } else if (error.code == ErrorCode::killSwitchNeedsRecovery()) {
        addActionButton(&box, QStringLiteral("Recover"), ErrorAction::RecoverKillSwitch, &actions);
        addActionButton(&box, QStringLiteral("Show Recovery Instructions"),
                        ErrorAction::ShowRecoveryInstructions, &actions);
        addActionButton(&box, QStringLiteral("Create Diagnostics"), ErrorAction::CreateDiagnostics,
                        &actions);
        box.addButton(QMessageBox::Cancel);
    } else if (error.code == ErrorCode::ruleSetMissing()) {
        addActionButton(&box, QStringLiteral("Open Rule Set Manager"), ErrorAction::OpenRuleSetManager,
                        &actions);
        addActionButton(&box, QStringLiteral("Continue"), ErrorAction::Continue, &actions);
        box.addButton(QMessageBox::Cancel);
    } else if (error.code == ErrorCode::geoDataMissing()) {
        addActionButton(&box, QStringLiteral("Open Geo Data Manager"), ErrorAction::OpenGeoDataManager,
                        &actions);
        addActionButton(&box, QStringLiteral("Continue"), ErrorAction::Continue, &actions);
        box.addButton(QMessageBox::Cancel);
    } else if (error.code == ErrorCode::profileUnsupportedRuntime()) {
        addActionButton(&box, QStringLiteral("Switch to Xray system proxy"),
                        ErrorAction::SwitchToSystemProxy, &actions);
        addActionButton(&box, QStringLiteral("Open Profile"), ErrorAction::OpenProfile, &actions);
        box.addButton(QMessageBox::Cancel);
    } else {
        box.setStandardButtons(QMessageBox::Ok);
    }

    if (includeCopyDiagnostics) {
        addActionButton(&box, QStringLiteral("Copy details"), ErrorAction::CopyDetails, &actions);
    }

    box.exec();
    QAbstractButton* clicked = box.clickedButton();
    if (clicked == box.button(QMessageBox::Ok)) {
        return ErrorAction::Ok;
    }
    if (clicked == box.button(QMessageBox::Cancel)) {
        return ErrorAction::Cancel;
    }
    if (actions.contains(clicked)) {
        if (actions.value(clicked) == ErrorAction::CopyDetails) {
            const QString clipboard = QStringLiteral("[%1] %2\n%3\n%4")
                                        .arg(error.code, error.title, error.message, error.details);
            QApplication::clipboard()->setText(clipboard);
        }
        return actions.value(clicked);
    }
    return ErrorAction::None;
}

} // namespace zarya
