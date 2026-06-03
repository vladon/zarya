#pragma once

#include "core/CoreManager.h"
#include "core/SingBoxAdapter.h"
#include "core/XrayAdapter.h"
#include "platform/SystemProxyController.h"
#include "storage/ProfileStore.h"
#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>

class QAction;
class QCloseEvent;
class QPlainTextEdit;
class QTableView;
class QToolBar;

namespace zarya {

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
    bool writeConfigFile(const QString& path, const QJsonObject& config, QString* error) const;
    ICoreAdapter* adapterFor(CoreType type);
    QString configPathFor(CoreType type) const;
    void loadProfilesOnStartup();

    bool confirmSystemProxyChangeIfNeeded();
    void tryAutoEnableSystemProxy();
    void tryRestoreSystemProxy(SystemProxyRestoreMode mode, bool showFailureDialog);
    QString coreStatusText() const;
    QString systemProxyStatusText() const;

    ProfileTableModel m_tableModel;
    ProfileStore m_profileStore;
    CoreManager m_coreManager;
    XrayAdapter m_xrayAdapter;
    SingBoxAdapter m_singBoxAdapter;
    SystemProxyController m_systemProxy;

    QTableView* m_tableView = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QToolBar* m_toolBar = nullptr;

    QAction* m_addAction = nullptr;
    QAction* m_editAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_importAction = nullptr;
    QAction* m_startAction = nullptr;
    QAction* m_stopAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_loadAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_enableSystemProxyAction = nullptr;
    QAction* m_restoreSystemProxyAction = nullptr;
};

} // namespace zarya
