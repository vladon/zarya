#pragma once

#include "storage/ProfileStore.h"
#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>

class QAction;
class QTableView;

namespace zarya {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onSaveProfiles();
    void onLoadProfiles();

private:
    void loadProfilesOnStartup();

    ProfileTableModel m_tableModel;
    ProfileStore m_profileStore;
    QTableView* m_tableView = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_loadAction = nullptr;
};

} // namespace zarya
