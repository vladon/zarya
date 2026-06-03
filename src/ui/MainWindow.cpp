#include "ui/MainWindow.h"

#include <QHeaderView>
#include <QTableView>

namespace zarya {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Zarya"));
    resize(960, 640);

    m_tableView = new QTableView(this);
    m_tableView->setModel(&m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);
    setCentralWidget(m_tableView);

    statusBar()->showMessage(QStringLiteral("Ready"));
}

} // namespace zarya
