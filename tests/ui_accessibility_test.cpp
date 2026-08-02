#include "ui/desktopapp/ProfileActionStrip.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"
#include "ui/desktopapp/ZaryaUiIntegration.h"

#include <QAccessible>
#include <QAccessibleActionInterface>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QWidget>
#include <iostream>
#include <utility>

namespace {

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

    return ok ? 0 : 1;
}
