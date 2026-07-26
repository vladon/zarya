#include "app/Application.h"

#include "app/StartupOptions.h"
#include "i18n/LanguageManager.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppPaths.h"

#include <QAbstractButton>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSystemTrayIcon>

namespace zarya {

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

    // Ok-only QMessageBox (information/warning) often has no Escape button on macOS.
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
    if (type == QEvent::KeyPress || type == QEvent::ShortcutOverride) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (auto* box = qobject_cast<QMessageBox*>(activeModalWidget())) {
                const QList<QAbstractButton*> buttons = box->buttons();
                // Ok-only info/warning boxes: Esc should always dismiss on macOS.
                // ShortcutOverride must be accepted or KeyPress may never arrive.
                if (buttons.size() == 1) {
                    if (type == QEvent::ShortcutOverride) {
                        keyEvent->accept();
                    }
                    buttons.first()->click();
                    return true;
                }
            }
        }
    }
    return QApplication::eventFilter(watched, event);
}

} // namespace zarya
