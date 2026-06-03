#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

namespace zarya {

class AppController;
class MainWindow;

class TrayController : public QObject {
    Q_OBJECT

public:
    TrayController(AppController* appController, MainWindow* mainWindow,
                   QObject* parent = nullptr);

    bool isAvailable() const;
    bool isActive() const;
    void showNotification(const QString& title, const QString& message);
    void updateMenuState();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupTray();
    QIcon createTrayIcon() const;

    AppController* m_appController = nullptr;
    MainWindow* m_mainWindow = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;

    QAction* m_showAction = nullptr;
    QAction* m_hideAction = nullptr;
    QAction* m_startAction = nullptr;
    QAction* m_stopAction = nullptr;
    QAction* m_enableProxyAction = nullptr;
    QAction* m_restoreProxyAction = nullptr;
    QAction* m_updateAllAction = nullptr;
    QAction* m_testSelectedAction = nullptr;
    QAction* m_exitAction = nullptr;
};

} // namespace zarya
