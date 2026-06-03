#pragma once

#include "app/AppController.h"
#include "core/CoreManager.h"
#include "core/SingBoxAdapter.h"
#include "core/XrayAdapter.h"
#include "platform/SystemProxyController.h"
#include "storage/ProfileStore.h"
#include "storage/SubscriptionStore.h"
#include "subscription/SubscriptionManager.h"
#include "testing/TestManager.h"
#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>
#include <QVector>

class QAction;
class QCloseEvent;
class QComboBox;
class QEvent;
class QPlainTextEdit;
class QSplitter;
class QTableView;
class QToolBar;

namespace zarya {

class TrayController;
struct Profile;
struct Subscription;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    bool hasSelectedProfile() const;
    bool isTestingBusy() const;
    bool isSubscriptionUpdateBusy() const;
    bool canRestoreSystemProxy() const;
    bool trayIsAvailable() const;

    AppController* appController();

public slots:
    void showFromTray();
    void hideToTray(bool notify = false);
    void startSelectedProfile();
    void testSelected();
    void updateAllSubscriptions();
    void requestApplicationQuit();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onAddProfile();
    void onEditProfile();
    void onDeleteProfile();
    void onImportVless();
    void onSaveProfiles();
    void onLoadProfiles();
    void onSettings();
    void onSubscriptions();
    void onUpdateSelectedSubscription();
    void onUpdateAllSubscriptions();
    void onProfileFilterChanged(int index);
    void onTestSelected();
    void onTestAll();
    void onTestTcpSelected();
    void onTestDelaySelected();
    void onCancelTests();
    void onTestProfileContext();
    void onTestTcpContext();
    void onTestDelayContext();
    void onTestStarted(const QString& profileId);
    void onProfileUpdated(const Profile& profile);
    void onTestProgressChanged(int done, int total);
    void onAllTestsFinished();
    void onStartCore();
    void onStopCore();
    void onEnableSystemProxy();
    void onRestoreSystemProxy();
    void onCoreStarted(const QString& coreName);
    void onCoreStopped();
    void onCoreLogLine(const QString& line);
    void onCoreError(const QString& message);
    void onAbout();
    void onQuitApproved();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void setupAppController();
    void setupTray();
    void appendLog(const QString& line);
    void updateStatusBar();
    void saveWindowState();
    void restoreWindowState();
    void selectProfileById(const QString& profileId);
    int selectedRow() const;
    int indexOfProfileById(const QString& profileId) const;
    Profile* selectedProfileInStorage();
    bool writeConfigFile(const QString& path, const QJsonObject& config, QString* error) const;
    ICoreAdapter* adapterFor(CoreType type);
    QString configPathFor(CoreType type) const;
    void loadAllOnStartup();
    bool saveAll(QString* errorMessage = nullptr);
    void refreshProfileFilterCombo();
    void refreshProfileView();
    QVector<QString> collectSelectedProfileIds() const;
    QVector<QString> collectAllTestableProfileIds() const;
    void startTestsForIds(const QVector<QString>& profileIds, TestMode mode);
    void setTestingUiBusy(bool busy);
    void showProfileContextMenu(const QPoint& position);
    bool shouldHideToTrayOnClose() const;

    bool confirmSystemProxyChangeIfNeeded();
    void tryAutoEnableSystemProxy();
    void tryRestoreSystemProxy(SystemProxyRestoreMode mode, bool showFailureDialog);
    QString coreStatusText() const;
    QString systemProxyStatusText() const;
    QString trayStatusText() const;
    void notifyTray(const QString& title, const QString& message);

    QVector<Profile> m_allProfiles;
    QVector<Subscription> m_subscriptions;
    QString m_profileFilterKey;

    ProfileTableModel m_tableModel;
    ProfileStore m_profileStore;
    SubscriptionStore m_subscriptionStore;
    SubscriptionManager m_subscriptionManager;
    TestManager m_testManager;
    CoreManager m_coreManager;
    XrayAdapter m_xrayAdapter;
    SingBoxAdapter m_singBoxAdapter;
    SystemProxyController m_systemProxy;
    AppController m_appController;
    TrayController* m_trayController = nullptr;

    QSplitter* m_splitter = nullptr;
    QTableView* m_tableView = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QToolBar* m_toolBar = nullptr;
    QComboBox* m_profileFilterCombo = nullptr;

    QAction* m_showAction = nullptr;
    QAction* m_hideToTrayAction = nullptr;
    QAction* m_exitAction = nullptr;
    QAction* m_addAction = nullptr;
    QAction* m_editAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_importAction = nullptr;
    QAction* m_startAction = nullptr;
    QAction* m_stopAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_loadAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_subscriptionsAction = nullptr;
    QAction* m_updateSubscriptionAction = nullptr;
    QAction* m_updateAllSubscriptionsAction = nullptr;
    QAction* m_enableSystemProxyAction = nullptr;
    QAction* m_restoreSystemProxyAction = nullptr;
    QAction* m_testSelectedAction = nullptr;
    QAction* m_testAllAction = nullptr;
    QAction* m_testTcpSelectedAction = nullptr;
    QAction* m_testDelaySelectedAction = nullptr;
    QAction* m_cancelTestsAction = nullptr;

    int m_testProgressDone = 0;
    int m_testProgressTotal = 0;
    bool m_trayCloseNotificationShown = false;
    bool m_subscriptionUpdateBusy = false;
    bool m_quitting = false;
};

} // namespace zarya
