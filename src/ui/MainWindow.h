#pragma once

#include "core/CoreManager.h"
#include "core/SingBoxAdapter.h"
#include "core/XrayAdapter.h"
#include "platform/SystemProxyController.h"
#include "storage/ProfileStore.h"
#include "storage/SubscriptionStore.h"
#include "subscription/SubscriptionManager.h"
#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>
#include <QVector>

class QAction;
class QCloseEvent;
class QComboBox;
class QPlainTextEdit;
class QTableView;
class QToolBar;

namespace zarya {

struct Profile;
struct Subscription;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

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
    void onStartCore();
    void onStopCore();
    void onEnableSystemProxy();
    void onRestoreSystemProxy();
    void onCoreStarted(const QString& coreName);
    void onCoreStopped();
    void onCoreLogLine(const QString& line);
    void onCoreError(const QString& message);
    void onAbout();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void appendLog(const QString& line);
    void updateStatusBar();
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

    bool confirmSystemProxyChangeIfNeeded();
    void tryAutoEnableSystemProxy();
    void tryRestoreSystemProxy(SystemProxyRestoreMode mode, bool showFailureDialog);
    QString coreStatusText() const;
    QString systemProxyStatusText() const;

    QVector<Profile> m_allProfiles;
    QVector<Subscription> m_subscriptions;
    QString m_profileFilterKey;

    ProfileTableModel m_tableModel;
    ProfileStore m_profileStore;
    SubscriptionStore m_subscriptionStore;
    SubscriptionManager m_subscriptionManager;
    CoreManager m_coreManager;
    XrayAdapter m_xrayAdapter;
    SingBoxAdapter m_singBoxAdapter;
    SystemProxyController m_systemProxy;

    QTableView* m_tableView = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QToolBar* m_toolBar = nullptr;
    QComboBox* m_profileFilterCombo = nullptr;

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
};

} // namespace zarya
