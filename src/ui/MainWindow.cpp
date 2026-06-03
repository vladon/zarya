#include "ui/MainWindow.h"

#include "platform/Platform.h"
#include "storage/AppPaths.h"
#include "ui/ProfileDialog.h"

#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonDocument>
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
    updateStatusBar();
    appendLog(QStringLiteral("Zarya started. Profiles: %1").arg(m_profileStore.filePath()));
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
    m_logView->setPlaceholderText(QStringLiteral("Core and application logs appear here…"));

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_tableView);
    splitter->addWidget(m_logView);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    statusBar()->showMessage(QStringLiteral("Ready"));
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

    auto* coreMenu = menuBar()->addMenu(QStringLiteral("&Core"));
    m_startAction = coreMenu->addAction(QStringLiteral("&Start"));
    m_stopAction = coreMenu->addAction(QStringLiteral("S&top"));
    m_stopAction->setEnabled(false);

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
    m_toolBar->addAction(m_startAction);
    m_toolBar->addAction(m_stopAction);
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
    connect(m_startAction, &QAction::triggered, this, &MainWindow::onStartCore);
    connect(m_stopAction, &QAction::triggered, this, &MainWindow::onStopCore);

    connect(&m_coreManager, &CoreManager::started, this, &MainWindow::onCoreStarted);
    connect(&m_coreManager, &CoreManager::stopped, this, &MainWindow::onCoreStopped);
    connect(&m_coreManager, &CoreManager::logLine, this, &MainWindow::onCoreLogLine);
    connect(&m_coreManager, &CoreManager::errorOccurred, this, &MainWindow::onCoreError);
}

void MainWindow::appendLog(const QString& line)
{
    m_logView->appendPlainText(line);
}

void MainWindow::updateStatusBar()
{
    if (m_coreManager.isRunning()) {
        statusBar()->showMessage(
            QStringLiteral("Running: %1").arg(m_coreManager.runningCoreName()));
        m_startAction->setEnabled(false);
        m_stopAction->setEnabled(true);
    } else {
        statusBar()->showMessage(QStringLiteral("Stopped"));
        m_startAction->setEnabled(true);
        m_stopAction->setEnabled(false);
    }
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

ICoreAdapter* MainWindow::adapterFor(CoreType type)
{
    switch (type) {
    case CoreType::Xray:
        return &m_xrayAdapter;
    case CoreType::SingBox:
        return &m_singBoxAdapter;
    }
    return nullptr;
}

QString MainWindow::coreExecutablePath(CoreType type) const
{
    switch (type) {
    case CoreType::Xray:
        return Platform::defaultXrayExecutablePath();
    case CoreType::SingBox:
        return Platform::defaultSingBoxExecutablePath();
    }
    return {};
}

QString MainWindow::configPathFor(CoreType type) const
{
    switch (type) {
    case CoreType::Xray:
        return AppPaths::xrayConfigPath();
    case CoreType::SingBox:
        return AppPaths::singBoxConfigPath();
    }
    return {};
}

bool MainWindow::writeConfigFile(const QString& path, const QJsonObject& config,
                                 QString* error) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
    return true;
}

void MainWindow::onStartCore()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Start core"),
                                 QStringLiteral("Select a profile to start."));
        return;
    }

    const Profile profile = m_tableModel.profileAt(row);
    if (!profile.enabled) {
        QMessageBox::information(this, QStringLiteral("Start core"),
                                 QStringLiteral("Selected profile is disabled."));
        return;
    }

    ICoreAdapter* adapter = adapterFor(profile.coreType);
    if (!adapter) {
        QMessageBox::warning(this, QStringLiteral("Start core"),
                             QStringLiteral("No adapter for selected core type."));
        return;
    }

    const ConfigGenerationResult generation = adapter->generateConfig(profile);
    if (!generation.success) {
        QMessageBox::warning(this, QStringLiteral("Config generation"), generation.errorMessage);
        appendLog(QStringLiteral("Config generation failed: %1").arg(generation.errorMessage));
        return;
    }

    const QString configPath = configPathFor(profile.coreType);
    QString writeError;
    if (!writeConfigFile(configPath, generation.config, &writeError)) {
        QMessageBox::warning(this, QStringLiteral("Config write"), writeError);
        appendLog(QStringLiteral("Failed to write config: %1").arg(writeError));
        return;
    }

    appendLog(QStringLiteral("Wrote config: %1").arg(configPath));

    const QString executablePath = coreExecutablePath(profile.coreType);
    if (!QFileInfo::exists(executablePath)) {
        const QString message =
            QStringLiteral("Core executable not found:\n%1\n\nPlace the binary under ./cores/")
                .arg(executablePath);
        QMessageBox::warning(this, QStringLiteral("Core not found"), message);
        appendLog(message);
        return;
    }

    const QStringList arguments = adapter->argumentsForConfig(configPath);
    appendLog(QStringLiteral("Starting %1: %2 %3")
                  .arg(adapter->displayName(), executablePath, arguments.join(QLatin1Char(' '))));
    m_coreManager.start(executablePath, adapter->displayName(), arguments);
}

void MainWindow::onStopCore()
{
    appendLog(QStringLiteral("Stopping core…"));
    m_coreManager.stop();
}

void MainWindow::onCoreStarted(const QString& coreName)
{
    appendLog(QStringLiteral("Core started: %1").arg(coreName));
    updateStatusBar();
}

void MainWindow::onCoreStopped()
{
    appendLog(QStringLiteral("Core stopped."));
    updateStatusBar();
}

void MainWindow::onCoreLogLine(const QString& line)
{
    appendLog(line);
}

void MainWindow::onCoreError(const QString& message)
{
    appendLog(QStringLiteral("Error: %1").arg(message));
    QMessageBox::warning(this, QStringLiteral("Core error"), message);
    updateStatusBar();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, QStringLiteral("About Zarya"),
                       QStringLiteral("Zarya 0.1.0\n\nNative proxy profile manager "
                                      "with external Xray / sing-box cores."));
}

} // namespace zarya
