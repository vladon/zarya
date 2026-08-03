#include "ui/MainWindowAccessibility.h"
#include "ui/ProfileDialog.h"
#include "ui/desktopapp/ProfileActionStrip.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"
#include "ui/desktopapp/ZaryaUiIntegration.h"

#include <QAccessible>
#include <QAccessibleActionInterface>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QPlainTextEdit>
#include <QStandardPaths>
#include <QTableView>
#include <QWidget>
#include <iostream>
#include <utility>

namespace {

QString capturedAnnouncement;
QAccessible::AnnouncementPoliteness capturedPoliteness =
    QAccessible::AnnouncementPoliteness::Polite;
QAccessible::UpdateHandler previousAccessibilityHandler = nullptr;

void captureAccessibilityUpdate(QAccessibleEvent* event)
{
    if (event->type() == QAccessible::Announcement) {
        auto* announcement = static_cast<QAccessibleAnnouncementEvent*>(event);
        capturedAnnouncement = announcement->message();
        capturedPoliteness = announcement->politeness();
    }
    if (previousAccessibilityHandler) {
        previousAccessibilityHandler(event);
    }
}

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

QAccessibleInterface* accessibleInterface(QWidget* widget)
{
    return widget ? QAccessible::queryAccessibleInterface(widget) : nullptr;
}

bool expectAccessible(
    QWidget* widget,
    QAccessible::Role role,
    const QString& name,
    const char* message)
{
    QAccessibleInterface* interface = accessibleInterface(widget);
    return expect(interface, message)
        && expect(interface->role() == role, message)
        && expect(interface->text(QAccessible::Name) == name, message);
}

QVector<QWidget*> accessibleRadioButtons(QWidget* group)
{
    QVector<QWidget*> result;
    const auto children = group->findChildren<QWidget*>(
        QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        QAccessibleInterface* interface = accessibleInterface(child);
        if (interface && interface->role() == QAccessible::RadioButton) {
            result.push_back(child);
        }
    }
    return result;
}

QWidget* findAccessibleWidget(
    QWidget* root,
    QAccessible::Role role,
    const QString& name)
{
    const auto widgets = root->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        QAccessibleInterface* interface = accessibleInterface(widget);
        if (interface && interface->role() == role
            && interface->text(QAccessible::Name) == name) {
            return widget;
        }
    }
    return nullptr;
}

QWidget* nextOrderedWidget(QWidget* widget, const QVector<QWidget*>& orderedWidgets)
{
    QWidget* candidate = widget->nextInFocusChain();
    while (candidate != widget) {
        if (orderedWidgets.contains(candidate)) {
            return candidate;
        }
        candidate = candidate->nextInFocusChain();
    }
    return nullptr;
}

} // namespace

int main(int argc, char** argv)
{
#if defined(Q_OS_LINUX)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif
    QStandardPaths::setTestModeEnabled(true);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ZaryaTests"));
    QCoreApplication::setApplicationName(QStringLiteral("Accessibility"));
    zarya::initDesktopAppUiIntegrations(argc, argv);

    bool ok = true;
    QWidget host;

    auto* action = new zarya::ZaryaActionButton(QStringLiteral("Apply"), &host);
    ok &= expect(action->focusProxy(), "action button should expose its inner focus target");
    ok &= expectAccessible(
        action->focusProxy(),
        QAccessible::Button,
        QStringLiteral("Apply"),
        "action button should expose its button role and name");

    bool activated = false;
    QObject::connect(action, &zarya::ZaryaActionButton::clicked, [&activated] {
        activated = true;
    });
    QAccessibleInterface* actionInterface = accessibleInterface(action->focusProxy());
    QAccessibleActionInterface* actionActions = actionInterface
        ? actionInterface->actionInterface()
        : nullptr;
    ok &= expect(actionActions, "action button should expose an accessibility action");
    if (actionActions) {
        actionActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(activated, "accessibility press should activate the action button");
    }

    QAction addAction(QStringLiteral("&Add"), &host);
    QAction importAction(QStringLiteral("&Import"), &host);
    QAction subscriptionsAction(QStringLiteral("&Subscriptions"), &host);
    QAction testAction(QStringLiteral("&Test"), &host);
    QAction startAction(QStringLiteral("&Start"), &host);
    QAction stopAction(QStringLiteral("S&top"), &host);
    zarya::ProfileActionStripActions stripActions;
    stripActions.add = &addAction;
    stripActions.importProfiles = &importAction;
    stripActions.subscriptions = &subscriptionsAction;
    stripActions.testSelected = &testAction;
    stripActions.start = &startAction;
    stripActions.stop = &stopAction;
    stripActions.overflow = {&addAction, &importAction, &subscriptionsAction};
    auto* actionStrip = new zarya::ProfileActionStrip(
        std::move(stripActions),
        QStringLiteral("Profile filter"),
        QStringLiteral("More…"),
        &host);
    actionStrip->profileSelector()->setItems(
        {{QStringLiteral("all"), QStringLiteral("All profiles")},
         {QStringLiteral("manual"), QStringLiteral("Manual")}},
        QStringLiteral("all"));
    ok &= expectAccessible(
        actionStrip->profileSelector()->focusProxy(),
        QAccessible::ButtonMenu,
        QStringLiteral("Profile filter: All profiles"),
        "profile action strip should expose a labelled profile selector");

    QWidget* moreButton = findAccessibleWidget(
        actionStrip, QAccessible::ButtonMenu, QStringLiteral("More…"));
    ok &= expect(moreButton, "profile action strip overflow should expose menu-button semantics");

    QWidget* addButton = findAccessibleWidget(
        actionStrip, QAccessible::Button, QStringLiteral("Add"));
    ok &= expect(addButton, "profile action strip should expose action button names");
    bool addTriggered = false;
    QObject::connect(&addAction, &QAction::triggered, [&addTriggered] {
        addTriggered = true;
    });
    QAccessibleInterface* addInterface = accessibleInterface(addButton);
    QAccessibleActionInterface* addActions = addInterface
        ? addInterface->actionInterface()
        : nullptr;
    ok &= expect(addActions, "profile action should expose an accessibility action");
    if (addActions) {
        addActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(addTriggered, "assistive press should trigger the profile action");
    }
    addAction.setText(QStringLiteral("&Create…"));
    ok &= expectAccessible(
        addButton,
        QAccessible::Button,
        QStringLiteral("Create…"),
        "profile action accessible name should follow QAction text changes");

    auto* profileTable = new QTableView(&host);
    auto* logFilter = new zarya::ZaryaSelector(&host);
    logFilter->setItems(
        {{QStringLiteral("all"), QStringLiteral("All")}},
        QStringLiteral("all"));
    auto* copyLogAction = new zarya::ZaryaActionButton(
        QStringLiteral("Copy selected"), &host);
    auto* clearLogAction = new zarya::ZaryaActionButton(
        QStringLiteral("Clear view"), &host);
    auto* logView = new QPlainTextEdit(&host);
    logView->setReadOnly(true);
    zarya::configureMainWindowDataAccessibility(
        profileTable,
        QStringLiteral("Profiles"),
        logFilter,
        QStringLiteral("Log filter"),
        {copyLogAction->focusProxy(), clearLogAction->focusProxy()},
        logView,
        QStringLiteral("Application log"));
    ok &= expectAccessible(
        profileTable,
        QAccessible::Table,
        QStringLiteral("Profiles"),
        "profile table should expose its purpose");
    ok &= expectAccessible(
        logFilter->focusProxy(),
        QAccessible::ButtonMenu,
        QStringLiteral("Log filter: All"),
        "log filter should expose its label and selected value");
    ok &= expectAccessible(
        logView,
        QAccessible::EditableText,
        QStringLiteral("Application log"),
        "application log should expose its purpose");
    QAccessibleInterface* logInterface = accessibleInterface(logView);
    ok &= expect(
        logInterface && logInterface->state().readOnly,
        "application log should remain exposed as read-only");
    ok &= expect(logView->tabChangesFocus(), "Tab should leave the read-only application log");
    const QVector<QWidget*> dataFocusOrder = {
        profileTable,
        logFilter->focusProxy(),
        copyLogAction->focusProxy(),
        clearLogAction->focusProxy(),
        logView,
    };
    ok &= expect(
        nextOrderedWidget(profileTable, dataFocusOrder) == logFilter->focusProxy(),
        "focus should move from profiles to the log filter");
    ok &= expect(
        nextOrderedWidget(logFilter->focusProxy(), dataFocusOrder)
            == copyLogAction->focusProxy(),
        "focus should move from the log filter to its first action");
    ok &= expect(
        nextOrderedWidget(copyLogAction->focusProxy(), dataFocusOrder)
            == clearLogAction->focusProxy(),
        "log actions should follow their visual order");
    ok &= expect(
        nextOrderedWidget(clearLogAction->focusProxy(), dataFocusOrder) == logView,
        "focus should reach the application log after its actions");

    auto* selector = new zarya::ZaryaSelector(&host);
    selector->setItems(
        {{QStringLiteral("xray"), QStringLiteral("Xray")},
         {QStringLiteral("singbox"), QStringLiteral("sing-box")}},
        QStringLiteral("xray"));
    auto* selectorRow = new zarya::ZaryaFormRow(QStringLiteral("Runtime"), selector, &host);
    ok &= expect(selector->focusProxy(), "selector should expose its inner focus target");
    ok &= expectAccessible(
        selector->focusProxy(),
        QAccessible::ButtonMenu,
        QStringLiteral("Runtime: Xray"),
        "selector should expose a labelled menu-button role");
    selector->setCurrentKey(QStringLiteral("singbox"));
    ok &= expectAccessible(
        selector->focusProxy(),
        QAccessible::ButtonMenu,
        QStringLiteral("Runtime: sing-box"),
        "selector accessibility name should follow the selected value");
    selectorRow->setLabel(QStringLiteral("Runtime mode"));
    ok &= expectAccessible(
        selector->focusProxy(),
        QAccessible::ButtonMenu,
        QStringLiteral("Runtime mode: sing-box"),
        "selector accessibility name should follow the form label");

    auto* textField = new zarya::ZaryaTextField(QStringLiteral("Example"), &host);
    auto* textRow = new zarya::ZaryaFormRow(QStringLiteral("Name"), textField, &host);
    ok &= expectAccessible(
        textField->focusProxy(),
        QAccessible::EditableText,
        QStringLiteral("Name"),
        "text field should use the form label as its accessible name");
    textRow->setLabel(QStringLiteral("Profile name"));
    ok &= expectAccessible(
        textField->focusProxy(),
        QAccessible::EditableText,
        QStringLiteral("Profile name"),
        "text field accessible name should follow form label changes");

    auto* textArea = new zarya::ZaryaTextArea(QStringLiteral("One value per line"), &host);
    new zarya::ZaryaFormRow(QStringLiteral("Values"), textArea, &host);
    ok &= expectAccessible(
        textArea->focusProxy(),
        QAccessible::EditableText,
        QStringLiteral("Values"),
        "text area should use the form label as its accessible name");

    auto* numberField = new zarya::ZaryaNumberField(QStringLiteral("1-10"), 1, 10, &host);
    new zarya::ZaryaFormRow(QStringLiteral("Retries"), numberField, &host);
    ok &= expectAccessible(
        numberField->focusProxy(),
        QAccessible::EditableText,
        QStringLiteral("Retries"),
        "number field should use the form label as its accessible name");

    auto* radioGroup = new zarya::ZaryaRadioGroup(0, &host);
    radioGroup->addOption(0, QStringLiteral("System proxy"));
    radioGroup->addOption(1, QStringLiteral("TUN"));
    const QVector<QWidget*> radioButtons = accessibleRadioButtons(radioGroup);
    ok &= expect(radioButtons.size() == 2, "radio group should expose each option");
    auto* radioRow = new zarya::ZaryaFormRow(
        QStringLiteral("Runtime mode"), radioGroup, &host);
    ok &= expectAccessible(
        radioGroup->focusProxy(),
        QAccessible::RadioButton,
        QStringLiteral("Runtime mode: System proxy"),
        "radio options should include their form-group label");
    if (radioButtons.size() == 2) {
        ok &= expectAccessible(
            radioButtons[1],
            QAccessible::RadioButton,
            QStringLiteral("Runtime mode: TUN"),
            "every radio option should include its form-group label");
        radioRow->setLabel(QStringLiteral("Connection mode"));
        ok &= expectAccessible(
            radioButtons[1],
            QAccessible::RadioButton,
            QStringLiteral("Connection mode: TUN"),
            "radio option names should follow form label changes");
    }

    auto* status = new zarya::ZaryaBodyText(QStringLiteral("Ready"), &host);
    auto* statusRow = new zarya::ZaryaFormRow(QStringLiteral("Status"), status, &host);
    ok &= expect(
        !statusRow->focusProxy(),
        "informational form rows should not expose a false focus target");

    zarya::ProfileDialog profileDialog;
    profileDialog.show();
    QCoreApplication::processEvents();
    ok &= expectAccessible(
        &profileDialog,
        QAccessible::Dialog,
        QStringLiteral("Profile"),
        "profile dialog should expose its window role and name");
    for (const QString& section : {
             QStringLiteral("Basic"),
             QStringLiteral("Transport"),
             QStringLiteral("TLS / REALITY"),
             QStringLiteral("Advanced")}) {
        ok &= expect(
            findAccessibleWidget(&profileDialog, QAccessible::Button, section),
            "profile dialog section tabs should expose their names");
    }
    QWidget* nameField = findAccessibleWidget(
        &profileDialog, QAccessible::EditableText, QStringLiteral("Name"));
    QWidget* saveButton = findAccessibleWidget(
        &profileDialog, QAccessible::Button, QStringLiteral("Save"));
    QWidget* cancelButton = findAccessibleWidget(
        &profileDialog, QAccessible::Button, QStringLiteral("Cancel"));
    ok &= expect(nameField, "profile dialog should expose its name field");
    ok &= expect(saveButton, "profile dialog should expose its Save action");
    ok &= expect(cancelButton, "profile dialog should expose its Cancel action");
    ok &= expect(
        QApplication::focusWidget() == nameField,
        "profile dialog should place initial focus on the first field");
    const QVector<QWidget*> profileActions = {cancelButton, saveButton};
    ok &= expect(
        nameField && nextOrderedWidget(nameField, profileActions) == cancelButton,
        "profile dialog focus order should reach Cancel before Save");
    ok &= expect(
        cancelButton && nextOrderedWidget(cancelButton, profileActions) == saveButton,
        "profile dialog focus order should place Save after Cancel");

    capturedAnnouncement.clear();
    capturedPoliteness = QAccessible::AnnouncementPoliteness::Polite;
    previousAccessibilityHandler = QAccessible::installUpdateHandler(
        captureAccessibilityUpdate);
    QAccessibleInterface* saveInterface = accessibleInterface(saveButton);
    QAccessibleActionInterface* saveActions = saveInterface
        ? saveInterface->actionInterface()
        : nullptr;
    ok &= expect(saveActions, "profile Save should expose an accessibility action");
    if (saveActions) {
        saveActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }
    QAccessible::installUpdateHandler(previousAccessibilityHandler);
    previousAccessibilityHandler = nullptr;
    const QString requiredMessage =
        QStringLiteral("Name and address are required; port must be 1–65535.");
    ok &= expectAccessible(
        findAccessibleWidget(
            &profileDialog, QAccessible::AlertMessage, requiredMessage),
        QAccessible::AlertMessage,
        requiredMessage,
        "profile validation should expose alert semantics");
    ok &= expect(
        capturedAnnouncement == requiredMessage,
        "profile validation should send an accessibility announcement");
    ok &= expect(
        capturedPoliteness == QAccessible::AnnouncementPoliteness::Assertive,
        "profile validation announcement should be assertive");
    ok &= expect(
        QApplication::focusWidget() == nameField,
        "invalid profile submission should return focus to the first missing field");

    bool profileRejected = false;
    QObject::connect(&profileDialog, &QDialog::rejected, [&profileRejected] {
        profileRejected = true;
    });
    QAccessibleInterface* cancelInterface = accessibleInterface(cancelButton);
    QAccessibleActionInterface* cancelActions = cancelInterface
        ? cancelInterface->actionInterface()
        : nullptr;
    ok &= expect(cancelActions, "profile Cancel should expose an accessibility action");
    if (cancelActions) {
        cancelActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(profileRejected, "assistive Cancel should reject the profile dialog");
    }

    return ok ? 0 : 1;
}
