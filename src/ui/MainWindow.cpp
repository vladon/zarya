#include "ui/MainWindow.h"

#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>

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

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    m_saveAction = fileMenu->addAction(QStringLiteral("&Save profiles"));
    m_loadAction = fileMenu->addAction(QStringLiteral("&Reload profiles"));

    auto* toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->addAction(m_saveAction);
    toolbar->addAction(m_loadAction);

    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveProfiles);
    connect(m_loadAction, &QAction::triggered, this, &MainWindow::onLoadProfiles);

    loadProfilesOnStartup();
    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::onSaveProfiles()
{
    QString error;
    if (!m_profileStore.save(m_tableModel.profiles(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("Profiles saved"), 3000);
}

void MainWindow::onLoadProfiles()
{
    QString error;
    const QVector<Profile> profiles = m_profileStore.load(&error);
    if (!error.isEmpty() && profiles.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Load failed"), error);
        return;
    }
    m_tableModel.setProfiles(profiles);
    statusBar()->showMessage(QStringLiteral("Loaded %1 profile(s)").arg(profiles.size()), 3000);
}

void MainWindow::loadProfilesOnStartup()
{
    QString error;
    const QVector<Profile> profiles = m_profileStore.load(&error);
    Q_UNUSED(error);
    m_tableModel.setProfiles(profiles);
}

} // namespace zarya
