#include "ui/TrayController.h"

#include "app/AppController.h"
#include "helperclient/HelperProcessManager.h"
#include "killswitch/KillSwitchState.h"
#include "storage/AppSettings.h"
#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

namespace zarya {

TrayController::TrayController(AppController* appController, MainWindow* mainWindow,
                               QObject* parent)
    : QObject(parent)
    , m_appController(appController)
    , m_mainWindow(mainWindow)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        if (m_appController) {
            emit m_appController->logLine(
                QStringLiteral("System tray is not available on this desktop environment."));
        }
        return;
    }

    setupTray();
    if (m_appController) {
        emit m_appController->logLine(QStringLiteral("Tray initialized"));
    }
}

bool TrayController::isAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable() && m_trayIcon != nullptr;
}

bool TrayController::isActive() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}

QIcon TrayController::createTrayIcon() const
{
    QIcon icon(QStringLiteral(":/icons/zarya.png"));
    if (!icon.isNull()) {
        return icon;
    }

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(0x33, 0x99, 0xdd));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 28, 28);
    painter.setBrush(QColor(0xff, 0xff, 0xff));
    painter.drawEllipse(10, 10, 12, 12);
    return QIcon(pixmap);
}

void TrayController::setupTray()
{
    m_trayIcon = new QSystemTrayIcon(createTrayIcon(), this);
    m_trayIcon->setToolTip(tr("Zarya"));

    m_trayMenu = new QMenu;
    m_showAction = m_trayMenu->addAction(tr("Show Zarya"));
    m_hideAction = m_trayMenu->addAction(tr("Hide Zarya"));
    m_trayMenu->addSeparator();
    m_startAction = m_trayMenu->addAction(tr("Start Selected Profile"));
    m_stopAction = m_trayMenu->addAction(tr("Stop"));
    m_enableProxyAction = m_trayMenu->addAction(tr("System Proxy: Enable"));
    m_restoreProxyAction = m_trayMenu->addAction(tr("System Proxy: Restore"));
    m_trayMenu->addSeparator();
    m_updateAllAction = m_trayMenu->addAction(tr("Update All Subscriptions"));
    m_testSelectedAction = m_trayMenu->addAction(tr("Test Selected Profile"));
    m_trayMenu->addSeparator();
    m_disableKillSwitchAction = m_trayMenu->addAction(tr("Kill Switch: Disable"));
    m_trayMenu->addSeparator();
    m_exitAction = m_trayMenu->addAction(tr("Exit"));

    connect(m_showAction, &QAction::triggered, m_mainWindow, &MainWindow::showFromTray);
    connect(m_hideAction, &QAction::triggered, m_mainWindow, &MainWindow::hideToTray);
    connect(m_startAction, &QAction::triggered, m_mainWindow, &MainWindow::startSelectedProfile);
    connect(m_stopAction, &QAction::triggered, m_appController, &AppController::stopCurrentProfile);
    connect(m_enableProxyAction, &QAction::triggered, m_appController,
            &AppController::enableSystemProxyManual);
    connect(m_restoreProxyAction, &QAction::triggered, m_appController,
            &AppController::restoreSystemProxyManual);
    connect(m_updateAllAction, &QAction::triggered, m_mainWindow,
            &MainWindow::updateAllSubscriptions);
    connect(m_testSelectedAction, &QAction::triggered, m_mainWindow, &MainWindow::testSelected);
    connect(m_disableKillSwitchAction, &QAction::triggered, this, [this]() {
        HelperProcessManager* helper = m_appController ? m_appController->helperProcessManager()
                                                       : nullptr;
        if (!helper) {
            return;
        }
        QString error;
        if (!helper->connectToHelper(&error)) {
            if (m_mainWindow) {
                m_mainWindow->appController()->logLine(
                    QStringLiteral("Kill switch disable failed: %1").arg(error));
            }
            return;
        }
        helper->killSwitchDisable(&error);
    });
    connect(m_exitAction, &QAction::triggered, m_appController, &AppController::requestQuit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayController::onTrayActivated);

    if (m_appController) {
        connect(m_appController, &AppController::coreStateChanged, this,
                &TrayController::updateMenuState);
        connect(m_appController, &AppController::proxyStateChanged, this,
                &TrayController::updateMenuState);
    }

    m_trayIcon->show();
    updateMenuState();

    QApplication::setWindowIcon(createTrayIcon());
}

void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick) {
        return;
    }
    if (!m_mainWindow) {
        return;
    }
    if (m_mainWindow->isVisible() && !m_mainWindow->isMinimized()) {
        m_mainWindow->hideToTray();
    } else {
        m_mainWindow->showFromTray();
    }
}

void TrayController::showNotification(const QString& title, const QString& message)
{
    if (!AppSettings::instance().showTrayNotifications() || !m_trayIcon) {
        return;
    }
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 4000);
}

void TrayController::updateMenuState()
{
    if (!m_trayMenu || !m_mainWindow || !m_appController) {
        return;
    }

    const bool coreRunning = m_appController->isCoreRunning();
    const bool hasSelection = m_mainWindow->hasSelectedProfile();
    const bool testsRunning = m_mainWindow->isTestingBusy();
    const bool updating = m_mainWindow->isSubscriptionUpdateBusy();

    m_startAction->setEnabled(hasSelection && !coreRunning && !testsRunning);
    m_stopAction->setEnabled(coreRunning);
    m_enableProxyAction->setEnabled(coreRunning);
    m_restoreProxyAction->setEnabled(m_mainWindow->canRestoreSystemProxy());
    m_updateAllAction->setEnabled(!updating);
    m_testSelectedAction->setEnabled(hasSelection && !testsRunning);

    bool showKillSwitchAction = false;
    if (HelperProcessManager* helper = m_appController->helperProcessManager()) {
        const KillSwitchStatus status = helper->killSwitchState().status;
        showKillSwitchAction = status == KillSwitchStatus::Enabled
                               || status == KillSwitchStatus::Failed
                               || status == KillSwitchStatus::NeedsRecovery;
    }
    if (m_disableKillSwitchAction) {
        m_disableKillSwitchAction->setVisible(showKillSwitchAction);
        m_disableKillSwitchAction->setEnabled(showKillSwitchAction);
    }
}

} // namespace zarya
