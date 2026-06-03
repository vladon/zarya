#pragma once

#include "storage/ProfileStore.h"
#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>

class QAction;
class QPlainTextEdit;
class QTableView;
class QToolBar;

namespace zarya {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAddProfile();
    void onEditProfile();
    void onDeleteProfile();
    void onSaveProfiles();
    void onLoadProfiles();
    void onAbout();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void appendLog(const QString& line);
    void loadProfilesOnStartup();
    int selectedRow() const;

    ProfileTableModel m_tableModel;
    ProfileStore m_profileStore;

    QTableView* m_tableView = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QToolBar* m_toolBar = nullptr;

    QAction* m_addAction = nullptr;
    QAction* m_editAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_loadAction = nullptr;
};

} // namespace zarya
