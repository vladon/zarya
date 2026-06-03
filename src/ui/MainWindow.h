#pragma once

#include "ui/models/ProfileTableModel.h"

#include <QMainWindow>

class QTableView;

namespace zarya {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    ProfileTableModel m_tableModel;
    QTableView* m_tableView = nullptr;
};

} // namespace zarya
