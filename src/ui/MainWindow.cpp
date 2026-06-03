#include "ui/MainWindow.h"

#include "domain/Profile.h"
#include "domain/Subscription.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/ImportVlessDialog.h"
#include "ui/ProfileDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/SubscriptionManagerDialog.h"

#include <QCloseEvent>
#include <QComboBox>
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
    connect(&m_subscriptionManager, &SubscriptionManager::logLine, this, &MainWindow::appendLog);

    loadAllOnStartup();
    updateStatusBar();
    appendLog(QStringLiteral("Zarya 0.4 started. Profiles: %1").arg(m_profileStore.filePath()));
    appendLog(QStringLiteral("Subscriptions: %1").arg(m_subscriptionStore.filePath()));
    appendLog(QStringLiteral("Xray path: %1").arg(AppSettings::instance().resolvedXrayPath()));
    if (!m_systemProxy.isSupported()) {
        appendLog(QStringLiteral("System proxy unsupported on this platform."));
    }
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
    m_settingsAction = fileMenu->addAction(QStringLiteral("&Settings…"));
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("E&xit"), this, &QWidget::close);

    auto* profileMenu = menuBar()->addMenu(QStringLiteral("&Profiles"));
    m_addAction = profileMenu->addAction(QStringLiteral("&Add…"));
    m_editAction = profileMenu->addAction(QStringLiteral("&Edit…"));
    m_deleteAction = profileMenu->addAction(QStringLiteral("&Delete"));
    profileMenu->addSeparator();
    m_importAction = profileMenu->addAction(QStringLiteral("Import &share links…"));

    auto* subscriptionsMenu = menuBar()->addMenu(QStringLiteral("Su&bscriptions"));
    m_subscriptionsAction = subscriptionsMenu->addAction(QStringLiteral("&Manage…"));
    m_updateSubscriptionAction =
        subscriptionsMenu->addAction(QStringLiteral("Update &Selected"));
    m_updateAllSubscriptionsAction =
        subscriptionsMenu->addAction(QStringLiteral("Update &All"));

    auto* coreMenu = menuBar()->addMenu(QStringLiteral("&Core"));
    m_startAction = coreMenu->addAction(QStringLiteral("&Start"));
    m_stopAction = coreMenu->addAction(QStringLiteral("S&top"));
    m_stopAction->setEnabled(false);

    auto* toolsMenu = menuBar()->addMenu(QStringLiteral("&Tools"));
    m_enableSystemProxyAction =
        toolsMenu->addAction(QStringLiteral("Enable &System Proxy"));
    m_restoreSystemProxyAction =
        toolsMenu->addAction(QStringLiteral("&Restore Previous Proxy"));

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
    m_toolBar->addAction(m_importAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_subscriptionsAction);
    m_toolBar->addAction(m_updateSubscriptionAction);
    m_toolBar->addAction(m_updateAllSubscriptionsAction);
    m_profileFilterCombo = new QComboBox(this);
    m_profileFilterCombo->setMinimumWidth(180);
    m_toolBar->addWidget(m_profileFilterCombo);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_startAction);
    m_toolBar->addAction(m_stopAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_enableSystemProxyAction);
    m_toolBar->addAction(m_restoreSystemProxyAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addAction(m_loadAction);
}

void MainWindow::setupConnections()
{
    connect(m_addAction, &QAction::triggered, this, &MainWindow::onAddProfile);
    connect(m_editAction, &QAction::triggered, this, &MainWindow::onEditProfile);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::onDeleteProfile);
    connect(m_importAction, &QAction::triggered, this, &MainWindow::onImportVless);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveProfiles);
    connect(m_loadAction, &QAction::triggered, this, &MainWindow::onLoadProfiles);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    connect(m_subscriptionsAction, &QAction::triggered, this, &MainWindow::onSubscriptions);
    connect(m_updateSubscriptionAction, &QAction::triggered, this,
            &MainWindow::onUpdateSelectedSubscription);
    connect(m_updateAllSubscriptionsAction, &QAction::triggered, this,
            &MainWindow::onUpdateAllSubscriptions);
    connect(m_profileFilterCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::onProfileFilterChanged);
    connect(m_startAction, &QAction::triggered, this, &MainWindow::onStartCore);
    connect(m_stopAction, &QAction::triggered, this, &MainWindow::onStopCore);
    connect(m_enableSystemProxyAction, &QAction::triggered, this,
            &MainWindow::onEnableSystemProxy);
    connect(m_restoreSystemProxyAction, &QAction::triggered, this,
            &MainWindow::onRestoreSystemProxy);

    connect(&m_coreManager, &CoreManager::started, this, &MainWindow::onCoreStarted);
    connect(&m_coreManager, &CoreManager::stopped, this, &MainWindow::onCoreStopped);
    connect(&m_coreManager, &CoreManager::logLine, this, &MainWindow::onCoreLogLine);
    connect(&m_coreManager, &CoreManager::errorOccurred, this, &MainWindow::onCoreError);
}

void MainWindow::appendLog(const QString& line)
{
    m_logView->appendPlainText(line);
}

QString MainWindow::coreStatusText() const
{
    return m_coreManager.isRunning() ? QStringLiteral("running") : QStringLiteral("stopped");
}

QString MainWindow::systemProxyStatusText() const
{
    return m_systemProxy.uiStatusText();
}

void MainWindow::updateStatusBar()
{
    const AppSettings& settings = AppSettings::instance();
    QString message =
        QStringLiteral("Core: %1 | System proxy: %2").arg(coreStatusText(), systemProxyStatusText());

    if (m_coreManager.isRunning()) {
        message += QStringLiteral(" | SOCKS 127.0.0.1:%1 | HTTP 127.0.0.1:%2")
                       .arg(settings.socksPort())
                       .arg(settings.httpPort());
        m_startAction->setEnabled(false);
        m_stopAction->setEnabled(true);
    } else {
        m_startAction->setEnabled(true);
        m_stopAction->setEnabled(false);
    }

    statusBar()->showMessage(message);

    const bool coreRunning = m_coreManager.isRunning();
    m_enableSystemProxyAction->setEnabled(coreRunning && m_systemProxy.isSupported()
                                        && !m_systemProxy.enabledByZarya());
    m_restoreSystemProxyAction->setEnabled(m_systemProxy.hasSavedState()
                                           && m_systemProxy.isSupported());
}

int MainWindow::selectedRow() const
{
    const QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first().row();
}

bool MainWindow::confirmSystemProxyChangeIfNeeded()
{
    if (!AppSettings::instance().confirmBeforeChangingSystemProxy()) {
        return true;
    }

    return QMessageBox::question(
               this, QStringLiteral("Change system proxy"),
               QStringLiteral("Zarya will change Windows system proxy settings. Continue?"))
           == QMessageBox::Yes;
}

void MainWindow::tryAutoEnableSystemProxy()
{
    const AppSettings& settings = AppSettings::instance();
    if (!settings.autoEnableSystemProxyOnStart() || !m_systemProxy.isSupported()) {
        return;
    }

    if (!confirmSystemProxyChangeIfNeeded()) {
        appendLog(QStringLiteral("System proxy change cancelled by user."));
        return;
    }

    QString error;
    const auto logLine = [this](const QString& line) { appendLog(line); };
    if (!m_systemProxy.enableLocalHttpProxy(settings.httpPort(), logLine, &error)) {
        QMessageBox::warning(this, QStringLiteral("System proxy"),
                             QStringLiteral("Failed to enable system proxy:\n%1").arg(error));
    }
    updateStatusBar();
}

void MainWindow::tryRestoreSystemProxy(SystemProxyRestoreMode mode, bool showFailureDialog)
{
    const bool wasEnabledByZarya = m_systemProxy.enabledByZarya();
    QString error;
    const auto logLine = [this](const QString& line) { appendLog(line); };
    const bool restored = m_systemProxy.restorePreviousProxy(mode, logLine, &error);

    if (!restored && showFailureDialog && !error.isEmpty()) {
        if (mode == SystemProxyRestoreMode::Manual
            || (mode == SystemProxyRestoreMode::Automatic && wasEnabledByZarya)) {
            QMessageBox::warning(this, QStringLiteral("System proxy"),
                                 QStringLiteral("Failed to restore system proxy:\n%1")
                                     .arg(error));
        }
    }

    updateStatusBar();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (AppSettings::instance().restoreProxyOnExit()) {
        tryRestoreSystemProxy(SystemProxyRestoreMode::Automatic, true);
    }

    if (m_coreManager.isRunning()) {
        appendLog(QStringLiteral("Stopping core on exit…"));
        m_coreManager.stop();
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::loadAllOnStartup()
{
    QString error;
    m_allProfiles = m_profileStore.load(&error);
    if (!error.isEmpty()) {
        appendLog(QStringLiteral("Startup profile load warning: %1").arg(error));
    }

    m_subscriptions = m_subscriptionStore.load(&error);
    if (!error.isEmpty()) {
        appendLog(QStringLiteral("Startup subscription load warning: %1").arg(error));
    }

    refreshProfileFilterCombo();
    refreshProfileView();
}

bool MainWindow::saveAll(QString* errorMessage)
{
    QString error;
    if (!m_profileStore.save(m_allProfiles, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    if (!m_subscriptionStore.save(m_subscriptions, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    return true;
}

void MainWindow::refreshProfileFilterCombo()
{
    const QString previous = m_profileFilterKey;
    m_profileFilterCombo->blockSignals(true);
    m_profileFilterCombo->clear();
    m_profileFilterCombo->addItem(QStringLiteral("All profiles"), QString());
    m_profileFilterCombo->addItem(QStringLiteral("Manual"), QStringLiteral("__manual__"));
    for (const Subscription& subscription : m_subscriptions) {
        m_profileFilterCombo->addItem(subscription.name, subscription.id);
    }

    const int index = m_profileFilterCombo->findData(previous);
    m_profileFilterCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_profileFilterKey = m_profileFilterCombo->currentData().toString();
    m_profileFilterCombo->blockSignals(false);
}

void MainWindow::refreshProfileView()
{
    QVector<Profile> visible;
    visible.reserve(m_allProfiles.size());
    for (const Profile& profile : m_allProfiles) {
        if (profile.deletedBySubscriptionUpdate) {
            continue;
        }
        if (m_profileFilterKey == QStringLiteral("__manual__") && !profile.isManual()) {
            continue;
        }
        if (!m_profileFilterKey.isEmpty() && m_profileFilterKey != QStringLiteral("__manual__")
            && profile.subscriptionId != m_profileFilterKey) {
            continue;
        }
        visible.append(profile);
    }
    m_tableModel.setProfiles(visible);
}

void MainWindow::onProfileFilterChanged(int index)
{
    Q_UNUSED(index);
    m_profileFilterKey = m_profileFilterCombo->currentData().toString();
    refreshProfileView();
}

int MainWindow::indexOfProfileById(const QString& profileId) const
{
    for (int i = 0; i < m_allProfiles.size(); ++i) {
        if (m_allProfiles.at(i).id == profileId) {
            return i;
        }
    }
    return -1;
}

Profile* MainWindow::selectedProfileInStorage()
{
    const int row = selectedRow();
    if (row < 0) {
        return nullptr;
    }
    const QString profileId = m_tableModel.profileAt(row).id;
    const int index = indexOfProfileById(profileId);
    if (index < 0) {
        return nullptr;
    }
    return &m_allProfiles[index];
}

void MainWindow::onSubscriptions()
{
    SubscriptionManagerDialog dialog(
        this, m_subscriptions, m_allProfiles, m_subscriptionManager, m_subscriptionStore,
        m_profileStore,
        [this](const QString& line) { appendLog(line); },
        [this]() {
            refreshProfileFilterCombo();
            refreshProfileView();
        });
    dialog.exec();
    refreshProfileFilterCombo();
    refreshProfileView();
}

void MainWindow::onUpdateSelectedSubscription()
{
    QString subscriptionId = m_profileFilterKey;
    if (subscriptionId.isEmpty() || subscriptionId == QStringLiteral("__manual__")) {
        QMessageBox::information(
            this, QStringLiteral("Update subscription"),
            QStringLiteral("Select a subscription in the profile filter, or use Subscriptions → "
                           "Manage to update a specific entry."));
        return;
    }

    int index = -1;
    for (int i = 0; i < m_subscriptions.size(); ++i) {
        if (m_subscriptions.at(i).id == subscriptionId) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        QMessageBox::warning(this, QStringLiteral("Update subscription"),
                             QStringLiteral("Subscription not found."));
        return;
    }

    const SubscriptionUpdateResult result =
        m_subscriptionManager.updateSubscription(m_subscriptions[index], m_allProfiles);
    QString error;
    if (!saveAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
    }
    refreshProfileFilterCombo();
    refreshProfileView();

    if (!result.success) {
        QMessageBox::warning(this, QStringLiteral("Update failed"), result.errorMessage);
    }
}

void MainWindow::onUpdateAllSubscriptions()
{
    const QVector<SubscriptionUpdateResult> results =
        m_subscriptionManager.updateAll(m_subscriptions, m_allProfiles);
    QString error;
    if (!saveAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
    }
    refreshProfileFilterCombo();
    refreshProfileView();

    int failed = 0;
    for (const SubscriptionUpdateResult& result : results) {
        if (!result.success) {
            ++failed;
        }
    }
    if (failed > 0) {
        QMessageBox::warning(this, QStringLiteral("Update all"),
                             QStringLiteral("%1 subscription(s) failed to update.").arg(failed));
    }
}

void MainWindow::onAddProfile()
{
    Profile profile = Profile::createVlessRealityDefault();
    if (!ProfileDialog::editProfile(this, profile)) {
        return;
    }
    m_allProfiles.append(profile);
    refreshProfileView();
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

    Profile* profile = selectedProfileInStorage();
    if (!profile || !ProfileDialog::editProfile(this, *profile)) {
        return;
    }
    refreshProfileView();
    appendLog(QStringLiteral("Updated profile: %1").arg(profile->name));
}

void MainWindow::onDeleteProfile()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Delete profile"),
                                 QStringLiteral("Select a profile first."));
        return;
    }

    const QString profileId = m_tableModel.profileAt(row).id;
    const int index = indexOfProfileById(profileId);
    if (index < 0) {
        return;
    }
    const QString name = m_allProfiles.at(index).name;
    if (QMessageBox::question(this, QStringLiteral("Delete profile"),
                              QStringLiteral("Delete profile \"%1\"?").arg(name))
        != QMessageBox::Yes) {
        return;
    }

    m_allProfiles.removeAt(index);
    refreshProfileView();
    appendLog(QStringLiteral("Deleted profile: %1").arg(name));
}

void MainWindow::onImportVless()
{
    ImportVlessDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QVector<Profile> imported = dialog.importedProfiles();
    for (const Profile& profile : imported) {
        m_allProfiles.append(profile);
        appendLog(QStringLiteral("Imported profile: %1").arg(profile.name));
    }

    refreshProfileView();
    onSaveProfiles();
}

void MainWindow::onSaveProfiles()
{
    QString error;
    if (!saveAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
        appendLog(QStringLiteral("Save failed: %1").arg(error));
        return;
    }
    appendLog(QStringLiteral("Profiles saved to %1").arg(m_profileStore.filePath()));
    appendLog(QStringLiteral("Subscriptions saved to %1").arg(m_subscriptionStore.filePath()));
}

void MainWindow::onLoadProfiles()
{
    QString error;
    m_allProfiles = m_profileStore.load(&error);
    if (!error.isEmpty() && m_allProfiles.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Load failed"), error);
        appendLog(QStringLiteral("Load failed: %1").arg(error));
        return;
    }

    m_subscriptions = m_subscriptionStore.load(&error);
    if (!error.isEmpty() && m_subscriptions.isEmpty()) {
        appendLog(QStringLiteral("Subscription load warning: %1").arg(error));
    }

    refreshProfileFilterCombo();
    refreshProfileView();
    appendLog(QStringLiteral("Loaded %1 profile(s) from %2")
                  .arg(m_allProfiles.size())
                  .arg(m_profileStore.filePath()));
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
    appendLog(QStringLiteral("Settings updated. Xray path: %1")
                  .arg(AppSettings::instance().resolvedXrayPath()));
    updateStatusBar();
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

    Profile* profilePtr = selectedProfileInStorage();
    if (!profilePtr) {
        return;
    }
    const Profile profile = *profilePtr;
    if (!profile.enabled) {
        QMessageBox::information(this, QStringLiteral("Start core"),
                                 QStringLiteral("Selected profile is disabled."));
        return;
    }

    if (profile.coreType != CoreType::Xray) {
        QMessageBox::information(this, QStringLiteral("Start core"),
                                 QStringLiteral("Only Xray profiles can be started in this "
                                                "milestone."));
        return;
    }

    ICoreAdapter* adapter = adapterFor(profile.coreType);
    if (!adapter) {
        QMessageBox::warning(this, QStringLiteral("Start core"),
                             QStringLiteral("No adapter for selected core type."));
        return;
    }

    appendLog(QStringLiteral("Generating config…"));

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

    appendLog(QStringLiteral("Config path: %1").arg(configPath));

    const QString executablePath = AppSettings::instance().resolvedXrayPath();
    if (!QFileInfo::exists(executablePath)) {
        const QString message =
            QStringLiteral("Xray executable not found:\n%1\n\nConfigure the path in Settings "
                           "(e.g. .\\cores\\xray\\xray.exe).")
                .arg(executablePath);
        QMessageBox::warning(this, QStringLiteral("Xray not found"), message);
        appendLog(message);
        return;
    }

    appendLog(QStringLiteral("Validating Xray config…"));
    const CoreValidationResult validation =
        m_coreManager.validateConfig(executablePath, configPath);
    if (!validation.output.isEmpty()) {
        appendLog(validation.output);
    }
    if (!validation.success) {
        QMessageBox::warning(this, QStringLiteral("Config validation failed"),
                             validation.errorMessage);
        appendLog(QStringLiteral("Validation failed."));
        return;
    }
    appendLog(QStringLiteral("Validation OK"));

    appendLog(QStringLiteral("Starting Xray…"));
    m_coreManager.startCore(executablePath, configPath, adapter->displayName());
}

void MainWindow::onStopCore()
{
    appendLog(QStringLiteral("Stopping core…"));
    tryRestoreSystemProxy(SystemProxyRestoreMode::Automatic, true);
    m_coreManager.stop();
}

void MainWindow::onEnableSystemProxy()
{
    if (!m_coreManager.isRunning()) {
        QMessageBox::information(
            this, QStringLiteral("System proxy"),
            QStringLiteral("Core is not running. Start a profile before enabling system proxy."));
        return;
    }

    if (!m_systemProxy.isSupported()) {
        QMessageBox::information(this, QStringLiteral("System proxy"),
                                 QStringLiteral("System proxy is not supported on this platform."));
        return;
    }

    if (!confirmSystemProxyChangeIfNeeded()) {
        return;
    }

    const AppSettings& settings = AppSettings::instance();
    QString error;
    const auto logLine = [this](const QString& line) { appendLog(line); };
    if (!m_systemProxy.enableLocalHttpProxy(settings.httpPort(), logLine, &error)) {
        QMessageBox::warning(this, QStringLiteral("System proxy"),
                             QStringLiteral("Failed to enable system proxy:\n%1").arg(error));
    }
    updateStatusBar();
}

void MainWindow::onRestoreSystemProxy()
{
    if (!m_systemProxy.hasSavedState()) {
        QMessageBox::information(
            this, QStringLiteral("System proxy"),
            QStringLiteral("No saved previous proxy state. Nothing to restore."));
        return;
    }

    tryRestoreSystemProxy(SystemProxyRestoreMode::Manual, true);
}

void MainWindow::onCoreStarted(const QString& coreName)
{
    const AppSettings& settings = AppSettings::instance();
    appendLog(QStringLiteral("%1 started").arg(coreName));
    appendLog(QStringLiteral("SOCKS: 127.0.0.1:%1").arg(settings.socksPort()));
    appendLog(QStringLiteral("HTTP: 127.0.0.1:%1").arg(settings.httpPort()));
    tryAutoEnableSystemProxy();
    updateStatusBar();
}

void MainWindow::onCoreStopped()
{
    tryRestoreSystemProxy(SystemProxyRestoreMode::Automatic, true);
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
    QMessageBox::about(
        this, QStringLiteral("About Zarya"),
        QStringLiteral(
            "Zarya 0.4\n\nNative proxy profile manager with Xray VLESS REALITY, Windows system "
            "proxy, and subscription import."));
}

} // namespace zarya
