#include "app/Application.h"

#include "app/StartupOptions.h"
#include "i18n/LanguageManager.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"

#include <QAbstractButton>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QTimer>

#if defined(Q_OS_MACOS)
#include "platform/macos/MacMessageBoxEscape.h"
#endif

namespace zarya {
namespace {

QAbstractButton* dismissButtonForMessageBox(QMessageBox* box)
{
    if (!box) {
        return nullptr;
    }
    if (QAbstractButton* button = box->escapeButton()) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Cancel)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Close)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::No)) {
        return button;
    }
    if (QAbstractButton* button = box->button(QMessageBox::Abort)) {
        return button;
    }

    const QList<QAbstractButton*> buttons = box->buttons();
    if (buttons.size() == 1) {
        return buttons.first();
    }

    QAbstractButton* rejectOrNo = nullptr;
    for (QAbstractButton* button : buttons) {
        const QMessageBox::ButtonRole role = box->buttonRole(button);
        if (role == QMessageBox::RejectRole || role == QMessageBox::NoRole) {
            if (rejectOrNo) {
                return nullptr;
            }
            rejectOrNo = button;
        }
    }
    return rejectOrNo;
}

void focusMessageBoxDismissControl(QMessageBox* box)
{
    if (!box) {
        return;
    }
    box->activateWindow();
    QAbstractButton* button = box->defaultButton();
    if (!button) {
        button = dismissButtonForMessageBox(box);
    }
    if (!button && !box->buttons().isEmpty()) {
        button = box->buttons().constFirst();
    }
    if (button) {
        button->setFocus(Qt::OtherFocusReason);
    }
}

} // namespace

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    setOrganizationName(QStringLiteral("Zarya"));
    setOrganizationDomain(QStringLiteral("zarya.app"));
    setApplicationName(QStringLiteral("Zarya"));
    setApplicationVersion(PackagingInfo::versionString());

    m_startupOptions = StartupOptionsParser::parse(*this);

    if (m_startupOptions.printVersionAndExit) {
        return;
    }

    AppPaths::initialize(m_startupOptions.portable);
    LanguageManager::instance().installTranslators();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        setQuitOnLastWindowClosed(false);
    }

#if defined(Q_OS_MACOS)
    // Sheets often have no first responder; Esc never becomes a Qt key event.
    installMacMessageBoxEscapeMonitor();
#endif

    // Focus + Esc fallback for Ok-only / Cancel message boxes.
    installEventFilter(this);
}

Application* Application::instance()
{
    return qobject_cast<Application*>(QApplication::instance());
}

const StartupOptions& Application::startupOptions() const
{
    return m_startupOptions;
}

bool Application::eventFilter(QObject* watched, QEvent* event)
{
    const QEvent::Type type = event->type();

    if (type == QEvent::Show) {
        if (auto* box = qobject_cast<QMessageBox*>(watched)) {
            // Defer until the native sheet/window is up so focus sticks on macOS.
            QTimer::singleShot(0, box, [box]() { focusMessageBoxDismissControl(box); });
        }
    }

    if (type == QEvent::KeyPress || type == QEvent::ShortcutOverride) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape || keyEvent->matches(QKeySequence::Cancel)) {
            QMessageBox* box = qobject_cast<QMessageBox*>(activeModalWidget());
            if (!box) {
                for (QObject* object = watched; object; object = object->parent()) {
                    box = qobject_cast<QMessageBox*>(object);
                    if (box) {
                        break;
                    }
                }
            }
            if (QAbstractButton* button = dismissButtonForMessageBox(box)) {
                if (type == QEvent::ShortcutOverride) {
                    keyEvent->accept();
                }
                button->click();
                return true;
            }
        }
    }
    return QApplication::eventFilter(watched, event);
}

} // namespace zarya
