#include "ui/BackupImportAccessibility.h"
#include "backup/BackupManager.h"
#include "ui/BetaBannerWidget.h"
#include "ui/BackupExportDialog.h"
#include "ui/MainWindowAccessibility.h"
#include "ui/CoreManagerAccessibility.h"
#include "ui/DnsManagerAccessibility.h"
#include "ui/DnsServerEditorDialog.h"
#include "ui/DiagnosticsPreviewDialog.h"
#include "ui/GeoDataManagerAccessibility.h"
#include "ui/ImportVlessDialog.h"
#include "ui/ProfileDialog.h"
#include "ui/RuleSetManagerAccessibility.h"
#include "ui/RoutingManagerAccessibility.h"
#include "ui/ReadinessDialog.h"
#include "ui/SafeExitDialog.h"
#include "ui/SubscriptionManagerAccessibility.h"
#include "ui/SubscriptionDialog.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/import/ProfileImportWidget.h"
#include "ui/onboarding/FirstRunAccessibility.h"
#include "ui/onboarding/FirstRunChecklistWidget.h"
#include "ui/onboarding/FirstRunState.h"
#include "ui/desktopapp/ProfileActionStrip.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"
#include "ui/desktopapp/ZaryaUiIntegration.h"

#include <QAccessible>
#include <QAccessibleActionInterface>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QEventLoop>
#include <QPlainTextEdit>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
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

bool focusIsWithin(QWidget* widget)
{
    QWidget* focused = QApplication::focusWidget();
    return widget && focused && (focused == widget || widget->isAncestorOf(focused));
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

void openDialogAndProcessDeferredFocus(QDialog* dialog)
{
    dialog->open();
    QEventLoop eventLoop;
    QTimer::singleShot(20, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    QCoreApplication::processEvents();
}

} // namespace

namespace zarya {

// BackupExportDialog's accessibility fixture never exports an archive. Keep its
// production constructor usable without linking the archive/persistence stack.
BackupManager::BackupManager(QObject* parent)
    : QObject(parent)
{
}

bool BackupManager::exportBackup(const BackupExportOptions&, QString* error)
{
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace zarya

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
    textArea->setReadOnly(true);
    QAccessibleInterface* textAreaInterface = accessibleInterface(textArea->focusProxy());
    ok &= expect(
        textAreaInterface && textAreaInterface->state().readOnly,
        "read-only text areas should expose their state");

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

    radioGroup->setAccessibleLabel(QStringLiteral("Backup type"));
    ok &= expectAccessible(
        radioGroup->focusProxy(),
        QAccessible::RadioButton,
        QStringLiteral("Backup type: System proxy"),
        "standalone radio groups should expose their semantic label");

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

    zarya::Subscription subscription;
    QTimer::singleShot(0, [&ok] {
        QTimer::singleShot(0, [&ok] {
            auto* subscriptionDialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!expect(subscriptionDialog, "subscription editor should become modal")) {
                return;
            }
            ok &= expectAccessible(
                subscriptionDialog,
                QAccessible::Dialog,
                QStringLiteral("Subscription"),
                "subscription editor should expose its dialog role and name");
            QWidget* subscriptionName = findAccessibleWidget(
                subscriptionDialog, QAccessible::EditableText, QStringLiteral("Name"));
            QWidget* subscriptionUrl = findAccessibleWidget(
                subscriptionDialog, QAccessible::EditableText, QStringLiteral("URL"));
            QWidget* subscriptionCancel = findAccessibleWidget(
                subscriptionDialog, QAccessible::Button, QStringLiteral("Cancel"));
            ok &= expect(subscriptionName, "subscription editor should expose its Name field");
            ok &= expect(subscriptionUrl, "subscription editor should expose its URL field");
            ok &= expect(subscriptionCancel, "subscription editor should expose its Cancel action");
            ok &= expect(
                QApplication::focusWidget() == subscriptionName,
                "subscription editor should initially focus Name");
            QAccessibleInterface* subscriptionCancelInterface =
                accessibleInterface(subscriptionCancel);
            QAccessibleActionInterface* subscriptionCancelActions = subscriptionCancelInterface
                ? subscriptionCancelInterface->actionInterface()
                : nullptr;
            ok &= expect(subscriptionCancelActions,
                         "subscription Cancel should expose an accessibility action");
            if (subscriptionCancelActions) {
                subscriptionCancelActions->doAction(QAccessibleActionInterface::pressAction());
            }
        });
    });
    ok &= expect(
        !zarya::SubscriptionDialog::editSubscription(&host, subscription),
        "assistive Cancel should reject the subscription editor");

    zarya::ImportVlessDialog importDialog;
    importDialog.show();
    QCoreApplication::processEvents();
    ok &= expectAccessible(
        &importDialog,
        QAccessible::Dialog,
        QStringLiteral("Import share links"),
        "share-link import should expose its dialog role and name");
    QWidget* linksField = findAccessibleWidget(
        &importDialog, QAccessible::EditableText, QStringLiteral("Share links"));
    QWidget* importButton = findAccessibleWidget(
        &importDialog, QAccessible::Button, QStringLiteral("Import"));
    QWidget* importCancelButton = findAccessibleWidget(
        &importDialog, QAccessible::Button, QStringLiteral("Cancel"));
    ok &= expect(linksField, "share-link import should expose its labelled input");
    ok &= expect(importButton, "share-link import should expose its Import action");
    ok &= expect(importCancelButton, "share-link import should expose its Cancel action");
    ok &= expect(
        QApplication::focusWidget() == linksField,
        "share-link import should initially focus its input");
    const QVector<QWidget*> importActions = {importCancelButton, importButton};
    ok &= expect(
        linksField && nextOrderedWidget(linksField, importActions) == importCancelButton,
        "share-link import focus order should reach Cancel before Import");
    ok &= expect(
        importCancelButton
            && nextOrderedWidget(importCancelButton, importActions) == importButton,
        "share-link import focus order should place Import after Cancel");

    QAccessibleInterface* importInterface = accessibleInterface(importButton);
    QAccessibleActionInterface* importActionsInterface = importInterface
        ? importInterface->actionInterface()
        : nullptr;
    ok &= expect(importActionsInterface, "Import should expose an accessibility action");
    capturedAnnouncement.clear();
    capturedPoliteness = QAccessible::AnnouncementPoliteness::Polite;
    previousAccessibilityHandler = QAccessible::installUpdateHandler(
        captureAccessibilityUpdate);
    if (importActionsInterface) {
        importActionsInterface->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }
    QAccessible::installUpdateHandler(previousAccessibilityHandler);
    previousAccessibilityHandler = nullptr;
    const QString noLinksMessage = QStringLiteral("No links to import.");
    ok &= expectAccessible(
        findAccessibleWidget(&importDialog, QAccessible::AlertMessage, noLinksMessage),
        QAccessible::AlertMessage,
        noLinksMessage,
        "empty share-link import should expose an inline alert");
    ok &= expect(
        capturedAnnouncement == noLinksMessage,
        "empty share-link import should announce its validation error");
    ok &= expect(
        capturedPoliteness == QAccessible::AnnouncementPoliteness::Assertive,
        "share-link validation errors should be assertive");
    ok &= expect(
        QApplication::focusWidget() == linksField,
        "empty share-link import should return focus to its input");

    auto* linksEditor = importDialog.findChild<zarya::ZaryaTextArea*>();
    ok &= expect(linksEditor, "share-link import should retain its text-area control");
    if (linksEditor && importActionsInterface) {
        linksEditor->setText(QStringLiteral("invalid://example"));
        capturedAnnouncement.clear();
        previousAccessibilityHandler = QAccessible::installUpdateHandler(
            captureAccessibilityUpdate);
        importActionsInterface->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QAccessible::installUpdateHandler(previousAccessibilityHandler);
        previousAccessibilityHandler = nullptr;
        ok &= expect(
            !capturedAnnouncement.isEmpty() && capturedAnnouncement != noLinksMessage,
            "invalid share-link import should announce the parser error inline");
        ok &= expectAccessible(
            findAccessibleWidget(
                &importDialog, QAccessible::AlertMessage, capturedAnnouncement),
            QAccessible::AlertMessage,
            capturedAnnouncement,
            "invalid share-link import should expose parser errors as alerts");
        ok &= expect(
            QApplication::focusWidget() == linksField,
            "invalid share-link import should return focus to its input");
    }

    bool importRejected = false;
    QObject::connect(&importDialog, &QDialog::rejected, [&importRejected] {
        importRejected = true;
    });
    QAccessibleInterface* importCancelInterface = accessibleInterface(importCancelButton);
    QAccessibleActionInterface* importCancelActions = importCancelInterface
        ? importCancelInterface->actionInterface()
        : nullptr;
    ok &= expect(importCancelActions, "import Cancel should expose an accessibility action");
    if (importCancelActions) {
        importCancelActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(importRejected, "assistive Cancel should reject share-link import");
    }

    zarya::ProfileImportWidget firstRunImport;
    firstRunImport.show();
    QCoreApplication::processEvents();
    QWidget* firstRunLinks = findAccessibleWidget(
        &firstRunImport, QAccessible::EditableText, QStringLiteral("Share links"));
    ok &= expect(firstRunLinks, "first-run import should expose its labelled input");
    ok &= expect(
        firstRunImport.focusProxy(),
        "first-run import should expose its text area as the page focus target");
    const QString initialImportSummary = QStringLiteral("Paste links to see parse summary.");
    ok &= expectAccessible(
        findAccessibleWidget(
            &firstRunImport, QAccessible::StaticText, initialImportSummary),
        QAccessible::StaticText,
        initialImportSummary,
        "first-run import should expose its initial parse summary");

    auto* firstRunEditor = firstRunImport.findChild<zarya::ZaryaTextArea*>();
    ok &= expect(firstRunEditor, "first-run import should retain its text-area control");
    const QString unsupportedSummary = QStringLiteral(
        "Parsed: VLESS 0, VMess 0, Trojan 0, Shadowsocks 0, Hysteria2 0, WireGuard 0, Unsupported 1");
    capturedAnnouncement.clear();
    capturedPoliteness = QAccessible::AnnouncementPoliteness::Assertive;
    previousAccessibilityHandler = QAccessible::installUpdateHandler(
        captureAccessibilityUpdate);
    if (firstRunEditor) {
        firstRunEditor->setText(QStringLiteral("invalid://example"));
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }
    QAccessible::installUpdateHandler(previousAccessibilityHandler);
    previousAccessibilityHandler = nullptr;
    ok &= expect(
        capturedAnnouncement == unsupportedSummary,
        "first-run import should announce updated parse statistics");
    ok &= expect(
        capturedPoliteness == QAccessible::AnnouncementPoliteness::Polite,
        "first-run parse statistics should use polite announcements");
    ok &= expectAccessible(
        findAccessibleWidget(
            &firstRunImport, QAccessible::StaticText, unsupportedSummary),
        QAccessible::StaticText,
        unsupportedSummary,
        "first-run import should expose the updated parse summary");

    QWidget wizardPage;
    wizardPage.setAccessibleName(QStringLiteral("Core setup"));
    auto* wizardAction = new zarya::ZaryaActionButton(
        QStringLiteral("Install Xray"), &wizardPage);
    wizardPage.show();
    wizardPage.activateWindow();
    QCoreApplication::processEvents();
    capturedAnnouncement.clear();
    capturedPoliteness = QAccessible::AnnouncementPoliteness::Assertive;
    previousAccessibilityHandler = QAccessible::installUpdateHandler(
        captureAccessibilityUpdate);
    zarya::activateFirstRunPageAccessibility(&wizardPage, wizardAction);
    QCoreApplication::processEvents();
    QAccessible::installUpdateHandler(previousAccessibilityHandler);
    previousAccessibilityHandler = nullptr;
    QWidget* expectedWizardFocus = wizardAction->focusProxy();
    ok &= expect(
        expectedWizardFocus && QApplication::focusWidget() == expectedWizardFocus,
        "first-run page activation should focus the page's first interactive control");
    ok &= expect(
        capturedAnnouncement == QStringLiteral("Core setup"),
        "first-run page activation should announce the page title");
    ok &= expect(
        capturedPoliteness == QAccessible::AnnouncementPoliteness::Polite,
        "first-run page titles should use polite announcements");

    zarya::FirstRunChecklistWidget checklist;
    checklist.show();
    QCoreApplication::processEvents();
    zarya::FirstRunState checklistState;
    checklistState.runtimeMode = zarya::RuntimeMode::SystemProxyXray;
    const QString checklistSummary = QStringLiteral(
        "Core: Xray missing\nProfiles: none\nRouting: selected\nDNS: selected\nRuntime: System proxy via Xray");
    capturedAnnouncement.clear();
    previousAccessibilityHandler = QAccessible::installUpdateHandler(
        captureAccessibilityUpdate);
    checklist.updateFromState(checklistState, 0, false, {});
    QCoreApplication::processEvents();
    QAccessible::installUpdateHandler(previousAccessibilityHandler);
    previousAccessibilityHandler = nullptr;
    ok &= expect(
        capturedAnnouncement == checklistSummary,
        "visible first-run checklist updates should announce their summary");
    ok &= expectAccessible(
        findAccessibleWidget(&checklist, QAccessible::StaticText, checklistSummary),
        QAccessible::StaticText,
        checklistSummary,
        "first-run checklist should expose its visible summary");

    zarya::RoutingJsonPreviewDialog routingPreview(
        QStringLiteral("{\"routing\":{}}"));
    openDialogAndProcessDeferredFocus(&routingPreview);
    ok &= expectAccessible(
        &routingPreview,
        QAccessible::Dialog,
        QStringLiteral("Xray Routing JSON Preview"),
        "routing preview should expose its dialog role and name");
    QWidget* routingJson = findAccessibleWidget(
        &routingPreview,
        QAccessible::EditableText,
        QStringLiteral("Generated routing JSON"));
    QWidget* routingClose = findAccessibleWidget(
        &routingPreview, QAccessible::Button, QStringLiteral("Close"));
    ok &= expect(routingJson, "routing preview should expose its labelled JSON");
    ok &= expect(routingClose, "routing preview should expose its Close action");
    QAccessibleInterface* routingJsonInterface = accessibleInterface(routingJson);
    ok &= expect(
        routingJsonInterface && routingJsonInterface->state().readOnly,
        "routing preview JSON should remain read-only");
    ok &= expect(
        focusIsWithin(routingJson),
        "routing preview should initially focus its JSON");
    ok &= expect(
        routingJson && nextOrderedWidget(routingJson, {routingClose}) == routingClose,
        "routing preview focus order should place Close after JSON");

    bool routingRejected = false;
    QObject::connect(&routingPreview, &QDialog::rejected, [&routingRejected] {
        routingRejected = true;
    });
    QAccessibleInterface* routingCloseInterface = accessibleInterface(routingClose);
    QAccessibleActionInterface* routingCloseActions = routingCloseInterface
        ? routingCloseInterface->actionInterface()
        : nullptr;
    ok &= expect(
        routingCloseActions,
        "routing preview Close should expose an accessibility action");
    if (routingCloseActions) {
        routingCloseActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(
            routingRejected,
            "assistive Close should reject the routing preview");
    }

    zarya::DiagnosticsPreviewResult diagnosticsResult;
    diagnosticsResult.files = {
        QStringLiteral("diagnostics.json"),
        QStringLiteral("logs/zarya.log")};
    diagnosticsResult.redactionMode = QStringLiteral("standard");
    zarya::DiagnosticsPreviewDialog diagnosticsPreview(diagnosticsResult);
    openDialogAndProcessDeferredFocus(&diagnosticsPreview);
    ok &= expectAccessible(
        &diagnosticsPreview,
        QAccessible::Dialog,
        QStringLiteral("Diagnostics Preview"),
        "diagnostics preview should expose its dialog role and name");
    QWidget* diagnosticsFiles = findAccessibleWidget(
        &diagnosticsPreview,
        QAccessible::EditableText,
        QStringLiteral("Included files"));
    QWidget* diagnosticsClose = findAccessibleWidget(
        &diagnosticsPreview, QAccessible::Button, QStringLiteral("Close"));
    ok &= expect(
        diagnosticsFiles,
        "diagnostics preview should expose its labelled file list");
    ok &= expect(
        diagnosticsClose,
        "diagnostics preview should expose its Close action");
    QAccessibleInterface* diagnosticsFilesInterface = accessibleInterface(diagnosticsFiles);
    ok &= expect(
        diagnosticsFilesInterface && diagnosticsFilesInterface->state().readOnly,
        "diagnostics preview file list should remain read-only");
    ok &= expect(
        focusIsWithin(diagnosticsFiles),
        "diagnostics preview should initially focus its file list");
    ok &= expect(
        diagnosticsFiles
            && nextOrderedWidget(diagnosticsFiles, {diagnosticsClose}) == diagnosticsClose,
        "diagnostics preview focus order should place Close after the file list");

    bool diagnosticsAccepted = false;
    QObject::connect(&diagnosticsPreview, &QDialog::accepted, [&diagnosticsAccepted] {
        diagnosticsAccepted = true;
    });
    QAccessibleInterface* diagnosticsCloseInterface = accessibleInterface(diagnosticsClose);
    QAccessibleActionInterface* diagnosticsCloseActions = diagnosticsCloseInterface
        ? diagnosticsCloseInterface->actionInterface()
        : nullptr;
    ok &= expect(
        diagnosticsCloseActions,
        "diagnostics preview Close should expose an accessibility action");
    if (diagnosticsCloseActions) {
        diagnosticsCloseActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(
            diagnosticsAccepted,
            "assistive Close should accept the diagnostics preview");
    }

    QDialog coreManagerHost;
    auto* coreTable = new QTableWidget(1, 1, &coreManagerHost);
    auto* checkVersions = new zarya::ZaryaActionButton(
        QStringLiteral("Check Versions"), &coreManagerHost);
    auto* updateSelected = new zarya::ZaryaActionButton(
        QStringLiteral("Update Selected"), &coreManagerHost);
    auto* coreLog = new QPlainTextEdit(&coreManagerHost);
    coreLog->setReadOnly(true);
    auto* coreManagerLayout = new QVBoxLayout(&coreManagerHost);
    coreManagerLayout->addWidget(coreTable);
    coreManagerLayout->addWidget(checkVersions);
    coreManagerLayout->addWidget(updateSelected);
    coreManagerLayout->addWidget(coreLog);
    zarya::configureCoreManagerAccessibility(
        coreTable,
        coreLog,
        {checkVersions, updateSelected},
        QStringLiteral("Installed cores"),
        QStringLiteral("Core manager log"));
    openDialogAndProcessDeferredFocus(&coreManagerHost);
    ok &= expectAccessible(
        coreTable,
        QAccessible::Table,
        QStringLiteral("Installed cores"),
        "Core Manager should expose its inventory table name and role");
    ok &= expectAccessible(
        coreLog,
        QAccessible::EditableText,
        QStringLiteral("Core manager log"),
        "Core Manager should expose its log name and text role");
    QAccessibleInterface* coreLogInterface = accessibleInterface(coreLog);
    ok &= expect(
        coreLogInterface && coreLogInterface->state().readOnly,
        "Core Manager log should remain read-only");
    ok &= expect(
        QApplication::focusWidget() == coreTable,
        "Core Manager should initially focus its inventory table");
    const QVector<QWidget*> coreManagerOrder = {
        checkVersions->focusProxy(),
        updateSelected->focusProxy(),
        coreLog};
    ok &= expect(
        nextOrderedWidget(coreTable, coreManagerOrder) == checkVersions->focusProxy(),
        "Core Manager focus order should place actions after the inventory table");
    ok &= expect(
        nextOrderedWidget(checkVersions->focusProxy(), coreManagerOrder)
            == updateSelected->focusProxy(),
        "Core Manager actions should retain their visual order");
    ok &= expect(
        nextOrderedWidget(updateSelected->focusProxy(), coreManagerOrder) == coreLog,
        "Core Manager focus order should place the log after its actions");

    QDialog geoDataManagerHost;
    auto* geoSource = new zarya::ZaryaSelector(&geoDataManagerHost);
    geoSource->setItems(
        {{QStringLiteral("official"), QStringLiteral("Official")}},
        QStringLiteral("official"));
    geoSource->setAccessibleLabel(QStringLiteral("Source"));
    auto* geoTable = new QTableWidget(1, 1, &geoDataManagerHost);
    auto* autoCheck = new zarya::ZaryaCheckBox(
        QStringLiteral("Check on startup"), &geoDataManagerHost);
    auto* warnMissing = new zarya::ZaryaCheckBox(
        QStringLiteral("Warn if missing"), &geoDataManagerHost);
    auto* geoLog = new QPlainTextEdit(&geoDataManagerHost);
    geoLog->setReadOnly(true);
    auto* checkGeoStatus = new zarya::ZaryaActionButton(
        QStringLiteral("Check Status"), &geoDataManagerHost);
    auto* closeGeoManager = new zarya::ZaryaActionButton(
        QStringLiteral("Close"), &geoDataManagerHost);
    auto* geoDataManagerLayout = new QVBoxLayout(&geoDataManagerHost);
    geoDataManagerLayout->addWidget(geoSource);
    geoDataManagerLayout->addWidget(geoTable);
    geoDataManagerLayout->addWidget(autoCheck);
    geoDataManagerLayout->addWidget(warnMissing);
    geoDataManagerLayout->addWidget(geoLog);
    geoDataManagerLayout->addWidget(checkGeoStatus);
    geoDataManagerLayout->addWidget(closeGeoManager);
    zarya::configureGeoDataManagerAccessibility(
        geoSource,
        geoTable,
        {autoCheck, warnMissing},
        geoLog,
        {checkGeoStatus, closeGeoManager},
        QStringLiteral("Geo data files"),
        QStringLiteral("Geo data log"));
    openDialogAndProcessDeferredFocus(&geoDataManagerHost);
    ok &= expectAccessible(
        geoTable,
        QAccessible::Table,
        QStringLiteral("Geo data files"),
        "Geo Data Manager should expose its inventory table name and role");
    ok &= expectAccessible(
        geoLog,
        QAccessible::EditableText,
        QStringLiteral("Geo data log"),
        "Geo Data Manager should expose its log name and text role");
    QAccessibleInterface* geoLogInterface = accessibleInterface(geoLog);
    ok &= expect(
        geoLogInterface && geoLogInterface->state().readOnly,
        "Geo Data Manager log should remain read-only");
    ok &= expect(
        QApplication::focusWidget() == geoSource->focusProxy(),
        "Geo Data Manager should initially focus its source selector");
    const QVector<QWidget*> geoDataManagerOrder = {
        geoTable,
        autoCheck->focusProxy(),
        warnMissing->focusProxy(),
        geoLog,
        checkGeoStatus->focusProxy(),
        closeGeoManager->focusProxy()};
    ok &= expect(
        nextOrderedWidget(geoSource->focusProxy(), geoDataManagerOrder) == geoTable,
        "Geo Data Manager should place the inventory after its source selector");
    ok &= expect(
        nextOrderedWidget(geoTable, geoDataManagerOrder) == autoCheck->focusProxy(),
        "Geo Data Manager should place options after its inventory");
    ok &= expect(
        nextOrderedWidget(warnMissing->focusProxy(), geoDataManagerOrder) == geoLog,
        "Geo Data Manager should place its log after options");
    ok &= expect(
        nextOrderedWidget(geoLog, geoDataManagerOrder) == checkGeoStatus->focusProxy(),
        "Geo Data Manager should place actions after its log");
    ok &= expect(
        nextOrderedWidget(checkGeoStatus->focusProxy(), geoDataManagerOrder)
            == closeGeoManager->focusProxy(),
        "Geo Data Manager actions should retain their visual order");

    QDialog ruleSetManagerHost;
    auto* requiredRuleSets = new QTableWidget(1, 1, &ruleSetManagerHost);
    auto* allRuleSets = new QTableWidget(1, 1, &ruleSetManagerHost);
    auto* ruleSetLog = new QPlainTextEdit(&ruleSetManagerHost);
    ruleSetLog->setReadOnly(true);
    auto* checkRuleSets = new zarya::ZaryaActionButton(
        QStringLiteral("Check Status"), &ruleSetManagerHost);
    auto* closeRuleSets = new zarya::ZaryaActionButton(
        QStringLiteral("Close"), &ruleSetManagerHost);
    auto* ruleSetManagerLayout = new QVBoxLayout(&ruleSetManagerHost);
    ruleSetManagerLayout->addWidget(requiredRuleSets);
    ruleSetManagerLayout->addWidget(allRuleSets);
    ruleSetManagerLayout->addWidget(ruleSetLog);
    ruleSetManagerLayout->addWidget(checkRuleSets);
    ruleSetManagerLayout->addWidget(closeRuleSets);
    zarya::configureRuleSetManagerAccessibility(
        requiredRuleSets,
        allRuleSets,
        ruleSetLog,
        {checkRuleSets, closeRuleSets},
        QStringLiteral("Required rule sets"),
        QStringLiteral("All rule sets"),
        QStringLiteral("Rule set log"));
    openDialogAndProcessDeferredFocus(&ruleSetManagerHost);
    ok &= expectAccessible(
        requiredRuleSets,
        QAccessible::Table,
        QStringLiteral("Required rule sets"),
        "Rule Set Manager should name its required table");
    ok &= expectAccessible(
        allRuleSets,
        QAccessible::Table,
        QStringLiteral("All rule sets"),
        "Rule Set Manager should name its complete inventory table");
    ok &= expectAccessible(
        ruleSetLog,
        QAccessible::EditableText,
        QStringLiteral("Rule set log"),
        "Rule Set Manager should name its log and preserve its text role");
    QAccessibleInterface* ruleSetLogInterface = accessibleInterface(ruleSetLog);
    ok &= expect(
        ruleSetLogInterface && ruleSetLogInterface->state().readOnly,
        "Rule Set Manager log should remain read-only");
    ok &= expect(
        QApplication::focusWidget() == requiredRuleSets,
        "Rule Set Manager should initially focus its visible required table");
    const QVector<QWidget*> ruleSetManagerOrder = {
        allRuleSets,
        ruleSetLog,
        checkRuleSets->focusProxy(),
        closeRuleSets->focusProxy()};
    ok &= expect(
        nextOrderedWidget(requiredRuleSets, ruleSetManagerOrder) == allRuleSets,
        "Rule Set Manager should order its tables by visual position");
    ok &= expect(
        nextOrderedWidget(allRuleSets, ruleSetManagerOrder) == ruleSetLog,
        "Rule Set Manager should place its log after its tables");
    ok &= expect(
        nextOrderedWidget(ruleSetLog, ruleSetManagerOrder) == checkRuleSets->focusProxy(),
        "Rule Set Manager should place actions after its log");

    requiredRuleSets->hide();
    zarya::configureRuleSetManagerAccessibility(
        requiredRuleSets,
        allRuleSets,
        ruleSetLog,
        {checkRuleSets, closeRuleSets},
        QStringLiteral("Required rule sets"),
        QStringLiteral("All rule sets"),
        QStringLiteral("Rule set log"));
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == allRuleSets,
        "Rule Set Manager should focus the all-items table when required items are empty");

    allRuleSets->hide();
    zarya::configureRuleSetManagerAccessibility(
        requiredRuleSets,
        allRuleSets,
        ruleSetLog,
        {checkRuleSets, closeRuleSets},
        QStringLiteral("Required rule sets"),
        QStringLiteral("All rule sets"),
        QStringLiteral("Rule set log"));
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == checkRuleSets->focusProxy(),
        "Rule Set Manager should focus its first action when both tables are empty");

    QDialog subscriptionManagerHost;
    auto* subscriptionsTable = new QTableView(&subscriptionManagerHost);
    auto* addSubscription = new zarya::ZaryaActionButton(
        QStringLiteral("Add"), &subscriptionManagerHost);
    auto* editSubscription = new zarya::ZaryaActionButton(
        QStringLiteral("Edit"), &subscriptionManagerHost);
    auto* closeSubscriptions = new zarya::ZaryaActionButton(
        QStringLiteral("Close"), &subscriptionManagerHost);
    auto* subscriptionManagerLayout = new QVBoxLayout(&subscriptionManagerHost);
    subscriptionManagerLayout->addWidget(subscriptionsTable);
    subscriptionManagerLayout->addWidget(addSubscription);
    subscriptionManagerLayout->addWidget(editSubscription);
    subscriptionManagerLayout->addWidget(closeSubscriptions);
    zarya::configureSubscriptionManagerAccessibility(
        subscriptionsTable,
        {addSubscription, editSubscription, closeSubscriptions},
        QStringLiteral("Subscriptions"));
    openDialogAndProcessDeferredFocus(&subscriptionManagerHost);
    ok &= expectAccessible(
        subscriptionsTable,
        QAccessible::Table,
        QStringLiteral("Subscriptions"),
        "Subscription Manager should expose its table name and role");
    ok &= expect(
        QApplication::focusWidget() == subscriptionsTable,
        "Subscription Manager should initially focus its visible table");
    const QVector<QWidget*> subscriptionManagerOrder = {
        addSubscription->focusProxy(),
        editSubscription->focusProxy(),
        closeSubscriptions->focusProxy()};
    ok &= expect(
        nextOrderedWidget(subscriptionsTable, subscriptionManagerOrder)
            == addSubscription->focusProxy(),
        "Subscription Manager should place Add after its table");
    ok &= expect(
        nextOrderedWidget(addSubscription->focusProxy(), subscriptionManagerOrder)
            == editSubscription->focusProxy(),
        "Subscription Manager actions should retain their visual order");

    subscriptionsTable->hide();
    zarya::configureSubscriptionManagerAccessibility(
        subscriptionsTable,
        {addSubscription, editSubscription, closeSubscriptions},
        QStringLiteral("Subscriptions"));
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == addSubscription->focusProxy(),
        "Subscription Manager should focus Add when its table is empty");

    QDialog routingManagerHost;
    auto* routingProfiles = new QTableWidget(1, 1, &routingManagerHost);
    auto* newRoutingProfile = new zarya::ZaryaActionButton(
        QStringLiteral("New"), &routingManagerHost);
    auto* editRoutingProfile = new zarya::ZaryaActionButton(
        QStringLiteral("Edit"), &routingManagerHost);
    auto* closeRoutingManager = new zarya::ZaryaActionButton(
        QStringLiteral("Close"), &routingManagerHost);
    auto* routingManagerLayout = new QVBoxLayout(&routingManagerHost);
    routingManagerLayout->addWidget(routingProfiles);
    routingManagerLayout->addWidget(newRoutingProfile);
    routingManagerLayout->addWidget(editRoutingProfile);
    routingManagerLayout->addWidget(closeRoutingManager);
    zarya::configureRoutingManagerAccessibility(
        routingProfiles,
        {newRoutingProfile, editRoutingProfile, closeRoutingManager},
        QStringLiteral("Routing Profiles"));
    openDialogAndProcessDeferredFocus(&routingManagerHost);
    ok &= expectAccessible(
        routingProfiles,
        QAccessible::Table,
        QStringLiteral("Routing Profiles"),
        "Routing Manager should expose its table name and role");
    ok &= expect(
        QApplication::focusWidget() == routingProfiles,
        "Routing Manager should initially focus its visible table");
    const QVector<QWidget*> routingManagerOrder = {
        newRoutingProfile->focusProxy(),
        editRoutingProfile->focusProxy(),
        closeRoutingManager->focusProxy()};
    ok &= expect(
        nextOrderedWidget(routingProfiles, routingManagerOrder)
            == newRoutingProfile->focusProxy(),
        "Routing Manager should place New after its table");
    ok &= expect(
        nextOrderedWidget(newRoutingProfile->focusProxy(), routingManagerOrder)
            == editRoutingProfile->focusProxy(),
        "Routing Manager actions should retain their visual order");

    routingProfiles->hide();
    zarya::configureRoutingManagerAccessibility(
        routingProfiles,
        {newRoutingProfile, editRoutingProfile, closeRoutingManager},
        QStringLiteral("Routing Profiles"));
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == newRoutingProfile->focusProxy(),
        "Routing Manager should focus New when its table is empty");

    QDialog dnsManagerHost;
    auto* dnsProfiles = new QTableWidget(1, 1, &dnsManagerHost);
    auto* newDnsProfile = new zarya::ZaryaActionButton(
        QStringLiteral("New"), &dnsManagerHost);
    auto* editDnsProfile = new zarya::ZaryaActionButton(
        QStringLiteral("Edit"), &dnsManagerHost);
    auto* closeDnsManager = new zarya::ZaryaActionButton(
        QStringLiteral("Close"), &dnsManagerHost);
    auto* dnsManagerLayout = new QVBoxLayout(&dnsManagerHost);
    dnsManagerLayout->addWidget(dnsProfiles);
    dnsManagerLayout->addWidget(newDnsProfile);
    dnsManagerLayout->addWidget(editDnsProfile);
    dnsManagerLayout->addWidget(closeDnsManager);
    zarya::configureDnsManagerAccessibility(
        dnsProfiles,
        {newDnsProfile, editDnsProfile, closeDnsManager},
        QStringLiteral("DNS Profiles"));
    openDialogAndProcessDeferredFocus(&dnsManagerHost);
    ok &= expectAccessible(
        dnsProfiles,
        QAccessible::Table,
        QStringLiteral("DNS Profiles"),
        "DNS Manager should expose its table name and role");
    ok &= expect(
        QApplication::focusWidget() == dnsProfiles,
        "DNS Manager should initially focus its visible table");
    const QVector<QWidget*> dnsManagerOrder = {
        newDnsProfile->focusProxy(),
        editDnsProfile->focusProxy(),
        closeDnsManager->focusProxy()};
    ok &= expect(
        nextOrderedWidget(dnsProfiles, dnsManagerOrder) == newDnsProfile->focusProxy(),
        "DNS Manager should place New after its table");
    ok &= expect(
        nextOrderedWidget(newDnsProfile->focusProxy(), dnsManagerOrder)
            == editDnsProfile->focusProxy(),
        "DNS Manager actions should retain their visual order");

    dnsProfiles->hide();
    zarya::configureDnsManagerAccessibility(
        dnsProfiles,
        {newDnsProfile, editDnsProfile, closeDnsManager},
        QStringLiteral("DNS Profiles"));
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == newDnsProfile->focusProxy(),
        "DNS Manager should focus New when its table is empty");

    QDialog backupImportHost;
    auto* backupPreview = new QTableWidget(1, 3, &backupImportHost);
    auto* profilesMode = new zarya::ZaryaSelector(&backupImportHost);
    auto* subscriptionsMode = new zarya::ZaryaSelector(&backupImportHost);
    auto* routingMode = new zarya::ZaryaSelector(&backupImportHost);
    auto* dnsMode = new zarya::ZaryaSelector(&backupImportHost);
    auto* settingsMode = new zarya::ZaryaSelector(&backupImportHost);
    for (zarya::ZaryaSelector* selector : {
             profilesMode, subscriptionsMode, routingMode, dnsMode, settingsMode}) {
        selector->setItems(
            {{QStringLiteral("merge"), QStringLiteral("Merge")}},
            QStringLiteral("merge"));
    }
    auto* machineSpecificSettings = new zarya::ZaryaCheckBox(
        QStringLiteral("Import machine-specific settings"), &backupImportHost);
    auto* browseBackup = new zarya::ZaryaActionButton(
        QStringLiteral("Browse"), &backupImportHost);
    auto* importBackup = new zarya::ZaryaActionButton(
        QStringLiteral("Import Selected"), &backupImportHost);
    auto* cancelBackupImport = new zarya::ZaryaActionButton(
        QStringLiteral("Cancel"), &backupImportHost);
    auto* backupImportLayout = new QVBoxLayout(&backupImportHost);
    backupImportLayout->addWidget(backupPreview);
    backupImportLayout->addWidget(profilesMode);
    backupImportLayout->addWidget(subscriptionsMode);
    backupImportLayout->addWidget(routingMode);
    backupImportLayout->addWidget(dnsMode);
    backupImportLayout->addWidget(settingsMode);
    backupImportLayout->addWidget(machineSpecificSettings);
    backupImportLayout->addWidget(browseBackup);
    backupImportLayout->addWidget(importBackup);
    backupImportLayout->addWidget(cancelBackupImport);
    zarya::configureBackupImportAccessibility(
        backupPreview,
        {profilesMode, subscriptionsMode, routingMode, dnsMode, settingsMode},
        machineSpecificSettings,
        browseBackup,
        importBackup,
        cancelBackupImport,
        QStringLiteral("Backup import preview"),
        false);
    backupImportHost.show();
    QCoreApplication::processEvents();
    ok &= expectAccessible(
        backupPreview,
        QAccessible::Table,
        QStringLiteral("Backup import preview"),
        "Backup Import should expose its preview table name and role");
    ok &= expect(
        QApplication::focusWidget() == browseBackup->focusProxy(),
        "Backup Import should initially focus Browse without a preview");
    zarya::configureBackupImportAccessibility(
        backupPreview,
        {profilesMode, subscriptionsMode, routingMode, dnsMode, settingsMode},
        machineSpecificSettings,
        browseBackup,
        importBackup,
        cancelBackupImport,
        QStringLiteral("Backup import preview"),
        true);
    QCoreApplication::processEvents();
    ok &= expect(
        QApplication::focusWidget() == backupPreview,
        "Backup Import should focus the preview table after loading an archive");
    const QVector<QWidget*> backupImportOrder = {
        profilesMode->focusProxy(),
        subscriptionsMode->focusProxy(),
        routingMode->focusProxy(),
        dnsMode->focusProxy(),
        settingsMode->focusProxy(),
        machineSpecificSettings->focusProxy(),
        browseBackup->focusProxy(),
        importBackup->focusProxy(),
        cancelBackupImport->focusProxy()};
    ok &= expect(
        nextOrderedWidget(backupPreview, backupImportOrder) == profilesMode->focusProxy(),
        "Backup Import should place mode selectors after the preview");
    ok &= expect(
        nextOrderedWidget(settingsMode->focusProxy(), backupImportOrder)
            == machineSpecificSettings->focusProxy(),
        "Backup Import should place machine-specific settings after modes");
    ok &= expect(
        nextOrderedWidget(machineSpecificSettings->focusProxy(), backupImportOrder)
            == browseBackup->focusProxy(),
        "Backup Import should place Browse after import settings");
    ok &= expect(
        nextOrderedWidget(browseBackup->focusProxy(), backupImportOrder)
            == importBackup->focusProxy(),
        "Backup Import should place Import after Browse");
    ok &= expect(
        nextOrderedWidget(importBackup->focusProxy(), backupImportOrder)
            == cancelBackupImport->focusProxy(),
        "Backup Import should place Cancel after Import");

    zarya::BackupManager backupManager;
    zarya::BackupExportDialog backupExportDialog(backupManager, {});
    openDialogAndProcessDeferredFocus(&backupExportDialog);
    ok &= expectAccessible(
        &backupExportDialog,
        QAccessible::Dialog,
        QStringLiteral("Export Backup"),
        "Backup Export should expose its dialog role and name");
    QWidget* backupType = findAccessibleWidget(
        &backupExportDialog,
        QAccessible::RadioButton,
        QStringLiteral("Backup type: Full configuration backup"));
    QWidget* backupOutput = findAccessibleWidget(
        &backupExportDialog, QAccessible::EditableText, QStringLiteral("Output"));
    ok &= expect(backupType, "Backup Export should expose its labelled backup type");
    ok &= expect(backupOutput, "Backup Export should expose its labelled output field");
    ok &= expect(
        QApplication::focusWidget() == backupType,
        "Backup Export should focus its selected backup type after opening");
    ok &= expect(
        backupType && nextOrderedWidget(backupType, {backupOutput}) == backupOutput,
        "Backup Export should place output after its backup type choices");
    backupExportDialog.reject();

    zarya::SafeExitDialog safeExitDialog;
    openDialogAndProcessDeferredFocus(&safeExitDialog);
    ok &= expectAccessible(
        &safeExitDialog,
        QAccessible::Dialog,
        QStringLiteral("Exit Zarya"),
        "Safe Exit should expose its dialog role and name");
    QWidget* stopRuntime = findAccessibleWidget(
        &safeExitDialog, QAccessible::CheckBox, QStringLiteral("Stop runtime"));
    QWidget* safeExit = findAccessibleWidget(
        &safeExitDialog, QAccessible::Button, QStringLiteral("Exit Safely"));
    ok &= expect(stopRuntime, "Safe Exit should expose the stop-runtime checkbox");
    ok &= expect(
        QApplication::focusWidget() == stopRuntime,
        "Safe Exit should focus Stop runtime after opening");
    ok &= expect(safeExit, "Safe Exit should expose its primary action");
    ok &= expect(
        stopRuntime && nextOrderedWidget(stopRuntime, {safeExit}) == safeExit,
        "Safe Exit should place its action row after recovery options");
    bool safeExitAccepted = false;
    QObject::connect(&safeExitDialog, &QDialog::accepted, [&safeExitAccepted] {
        safeExitAccepted = true;
    });
    QAccessibleInterface* safeExitInterface = accessibleInterface(safeExit);
    QAccessibleActionInterface* safeExitActions = safeExitInterface
        ? safeExitInterface->actionInterface()
        : nullptr;
    ok &= expect(safeExitActions, "Safe Exit action should expose an accessibility action");
    if (safeExitActions) {
        safeExitActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(safeExitAccepted, "assistive Exit Safely should accept the dialog");
    }

    zarya::ReadinessDialog readinessDialog;
    openDialogAndProcessDeferredFocus(&readinessDialog);
    ok &= expectAccessible(
        &readinessDialog,
        QAccessible::Dialog,
        QStringLiteral("Zarya Setup"),
        "Readiness should expose its dialog role and name");
    QWidget* coreManager = findAccessibleWidget(
        &readinessDialog, QAccessible::Button, QStringLiteral("Open Core Manager"));
    ok &= expect(coreManager, "Readiness should expose Open Core Manager");
    ok &= expect(
        QApplication::focusWidget() == coreManager,
        "Readiness should focus Open Core Manager after opening");
    QWidget* importProfile = findAccessibleWidget(
        &readinessDialog, QAccessible::Button, QStringLiteral("Import Profile"));
    ok &= expect(importProfile, "Readiness should expose Import Profile");
    ok &= expect(
        coreManager && nextOrderedWidget(coreManager, {importProfile}) == importProfile,
        "Readiness should place Import Profile after Open Core Manager");
    bool coreManagerRequested = false;
    QObject::connect(
        &readinessDialog, &zarya::ReadinessDialog::openCoreManagerRequested,
        [&coreManagerRequested] { coreManagerRequested = true; });
    QAccessibleInterface* coreManagerInterface = accessibleInterface(coreManager);
    QAccessibleActionInterface* coreManagerActions = coreManagerInterface
        ? coreManagerInterface->actionInterface()
        : nullptr;
    ok &= expect(
        coreManagerActions,
        "Readiness Open Core Manager should expose an accessibility action");
    if (coreManagerActions) {
        coreManagerActions->doAction(QAccessibleActionInterface::pressAction());
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        ok &= expect(
            coreManagerRequested,
            "assistive Open Core Manager should request the recovery workflow");
    }

    zarya::BetaBannerWidget betaBanner;
    ok &= expect(
        !betaBanner.accessibleName().isEmpty(),
        "Beta banner should expose its warning text as an accessible name");
    ok &= expect(
        betaBanner.focusProxy(),
        "Beta banner should expose Dismiss as its focus target");

    zarya::DnsServerEditorDialog dnsServerDialog(zarya::DnsServer{});
    dnsServerDialog.show();
    QCoreApplication::processEvents();
    QWidget* dnsAddress = findAccessibleWidget(
        &dnsServerDialog, QAccessible::EditableText, QStringLiteral("Address"));
    ok &= expect(dnsAddress, "DNS Server editor should expose its address field");
    ok &= expect(
        QApplication::focusWidget() == dnsAddress,
        "DNS Server editor should initially focus Address");

    return ok ? 0 : 1;
}
