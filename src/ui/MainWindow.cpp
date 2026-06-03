#include "ui/MainWindow.h"

#include "storage/AppPaths.h"
#include "ui/ProfileDialog.h"

#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>

namespace zarya {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupConnections();
    loadProfilesOnStartup();
    appendLog(QStringLiteral("Zarya started. Profiles: %1").arg(m_profileStore.filePath()));
    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Zarya"));
    resize(960, 640);

    m_tableView = new QTableView(this);
    m_tableView->setModel(&m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setPlaceholderText(QStringLiteral("Application logs appear here…"));

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_tableView);
    splitter->addWidget(m_logView);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    m_saveAction = fileMenu->addAction(QStringLiteral("&Save profiles"));
    m_loadAction = fileMenu->addAction(QStringLiteral("&Reload profiles"));
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

    auto* profileMenu = menuBar()->addMenu(QStringLiteral("&Profiles"));
    m_addAction = profileMenu->addAction(QStringLiteral("&Add…"));
    m_editAction = profileMenu->addAction(QStringLiteral("&Edit…"));
    m_deleteAction = profileMenu->addAction(QStringLiteral("&Delete"));

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(QStringLiteral("&About"), this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(QStringLiteral("Main"));
    m_toolBar->setMovable(false);
    m_toolBar->addAction(m_addAction);
    m_toolBar->addAction(m_editAction);
    m_toolBar->addAction(m_deleteAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addAction(m_loadAction);
}

void MainWindow::setupConnections()
{
    connect(m_addAction, &QAction::triggered, this, &MainWindow::onAddProfile);
    connect(m_editAction, &QAction::triggered, this, &MainWindow::onEditProfile);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::onDeleteProfile);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveProfiles);
    connect(m_loadAction, &QAction::triggered, this, &MainWindow::onLoadProfiles);
}

void MainWindow::appendLog(const QString& line)
{
    m_logView->appendPlainText(line);
}

int MainWindow::selectedRow() const
{
    const QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first().row();
}

void MainWindow::onAddProfile()
{
    Profile profile = Profile::createDefault();
    if (!ProfileDialog::editProfile(this, profile)) {
        return;
    }
    m_tableModel.addProfile(profile);
    appendLog(QStringLiteral("Added profile: %1").arg(profile.name));
}

void MainWindow::onEditProfile()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Edit profile"),
                                 QStringLiteral("Select a profile first."));
        return;
    }

    Profile profile = m_tableModel.profileAt(row);
    if (!ProfileDialog::editProfile(this, profile)) {
        return;
    }
    m_tableModel.updateProfile(row, profile);
    appendLog(QStringLiteral("Updated profile: %1").arg(profile.name));
}

void MainWindow::onDeleteProfile()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Delete profile"),
                                 QStringLiteral("Select a profile first."));
        return;
    }

    const QString name = m_tableModel.profileAt(row).name;
    if (QMessageBox::question(this, QStringLiteral("Delete profile"),
                              QStringLiteral("Delete profile \"%1\"?").arg(name))
        != QMessageBox::Yes) {
        return;
    }

    m_tableModel.removeProfile(row);
    appendLog(QStringLiteral("Deleted profile: %1").arg(name));
}

void MainWindow::onSaveProfiles()
{
    QString error;
    if (!m_profileStore.save(m_tableModel.profiles(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
        appendLog(QStringLiteral("Save failed: %1").arg(error));
        return;
    }
    appendLog(QStringLiteral("Profiles saved to %1").arg(m_profileStore.filePath()));
}

void MainWindow::onLoadProfiles()
{
    QString error;
    const QVector<Profile> profiles = m_profileStore.load(&error);
    if (!error.isEmpty() && profiles.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Load failed"), error);
        appendLog(QStringLiteral("Load failed: %1").arg(error));
        return;
    }
    m_tableModel.setProfiles(profiles);
    appendLog(QStringLiteral("Loaded %1 profile(s) from %2")
                  .arg(profiles.size())
                  .arg(m_profileStore.filePath()));
}

void MainWindow::loadProfilesOnStartup()
{
    QString error;
    const QVector<Profile> profiles = m_profileStore.load(&error);
    if (!error.isEmpty()) {
        appendLog(QStringLiteral("Startup load warning: %1").arg(error));
    }
    m_tableModel.setProfiles(profiles);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, QStringLiteral("About Zarya"),
                       QStringLiteral("Zarya 0.1.0\n\nNative proxy profile manager."));
}

} // namespace zarya
