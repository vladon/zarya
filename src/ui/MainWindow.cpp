#include "ui/MainWindow.h"

#include "ui/TrayController.h"

#include "domain/Profile.h"
#include "domain/ProtocolType.h"
#include "domain/Subscription.h"
#include "app/StartupOptions.h"
#include "packaging/PackagingInfo.h"
#include "helperclient/HelperProcessManager.h"
#include "killswitch/KillSwitchState.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/ImportVlessDialog.h"
#include "ui/ProfileDialog.h"
#include "ui/DnsManagerDialog.h"
#include "ui/GeoDataManagerDialog.h"
#include "ui/RuleSetManagerDialog.h"
#include "ui/RoutingManagerDialog.h"
#include "ui/SettingsDialog.h"
#include "geodata/GeoDataFileStatus.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/GeoDataSettingsStore.h"
#include "ui/SubscriptionManagerDialog.h"
#include "ui/SingBoxConfigPreviewDialog.h"

#include <QApplication>
#include <QJsonDocument>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonDocument>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>

namespace zarya {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_appController(&m_coreManager, &m_systemProxy, &m_xrayAdapter, &m_testManager,
                      &m_routingManager, &m_geoDataManager, &m_dnsManager, &m_ruleSetManager,
                      this)
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupAppController();
    setupConnections();
    setupTray();
    connect(&m_subscriptionManager, &SubscriptionManager::logLine, this, &MainWindow::appendLog);

    restoreWindowState();
    loadAllOnStartup();
    updateStatusBar();
    appendLog(QStringLiteral("Zarya %1 started. Profiles: %2")
                  .arg(PackagingInfo::versionString(), m_profileStore.filePath()));
    appendLog(QStringLiteral("System proxy backend: %1 (support: %2)")
                  .arg(m_systemProxy.backendName(), m_systemProxy.supportLevel()));
    if (!m_systemProxy.limitations().isEmpty()) {
        appendLog(m_systemProxy.limitations());
    }
    appendLog(QStringLiteral("Routing: %1").arg(m_routingManager.filePath()));
    appendLog(QStringLiteral("DNS: %1").arg(m_dnsManager.filePath()));
    appendLog(QStringLiteral("Active routing profile: %1")
                  .arg(m_routingManager.activeProfile().name));
    checkGeoDataOnStartup();
    checkUncleanTunShutdownWarning();
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
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested, this,
            &MainWindow::showProfileContextMenu);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setPlaceholderText(QStringLiteral("Core and application logs appear here…"));

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->addWidget(m_tableView);
    m_splitter->addWidget(m_logView);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);
    setCentralWidget(m_splitter);

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
    m_showAction = fileMenu->addAction(QStringLiteral("&Show"));
    m_hideToTrayAction = fileMenu->addAction(QStringLiteral("Hide to &Tray"));
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction(QStringLiteral("E&xit"));
    connect(m_showAction, &QAction::triggered, this, &MainWindow::showFromTray);
    connect(m_hideToTrayAction, &QAction::triggered, this, [this]() { hideToTray(false); });
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::requestApplicationQuit);

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

    auto* testMenu = menuBar()->addMenu(QStringLiteral("&Test"));
    m_testSelectedAction = testMenu->addAction(QStringLiteral("Test &Selected"));
    m_testAllAction = testMenu->addAction(QStringLiteral("Test &All"));
    testMenu->addSeparator();
    m_testTcpSelectedAction = testMenu->addAction(QStringLiteral("Test &TCP Selected"));
    m_testDelaySelectedAction = testMenu->addAction(QStringLiteral("Test &Delay Selected"));
    testMenu->addSeparator();
    m_cancelTestsAction = testMenu->addAction(QStringLiteral("&Cancel Tests"));
    m_cancelTestsAction->setEnabled(false);

    auto* coreMenu = menuBar()->addMenu(QStringLiteral("&Core"));
    m_startAction = coreMenu->addAction(QStringLiteral("&Start"));
    m_stopAction = coreMenu->addAction(QStringLiteral("S&top"));
    m_stopAction->setEnabled(false);

    auto* toolsMenu = menuBar()->addMenu(QStringLiteral("&Tools"));
    m_routingProfilesAction =
        toolsMenu->addAction(QStringLiteral("Routing &Profiles…"));
    m_geoDataManagerAction = toolsMenu->addAction(QStringLiteral("Geo Data &Manager…"));
    m_ruleSetManagerAction = toolsMenu->addAction(QStringLiteral("sing-box Rule &Sets…"));
    m_dnsProfilesAction = toolsMenu->addAction(QStringLiteral("DNS &Profiles…"));
    m_previewSingBoxTunConfigAction =
        toolsMenu->addAction(QStringLiteral("Preview sing-box TUN config…"));
    toolsMenu->addSeparator();
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
    m_toolBar->addAction(m_testSelectedAction);
    m_toolBar->addAction(m_testAllAction);
    m_toolBar->addAction(m_testTcpSelectedAction);
    m_toolBar->addAction(m_testDelaySelectedAction);
    m_toolBar->addAction(m_cancelTestsAction);
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
    connect(m_routingProfilesAction, &QAction::triggered, this, &MainWindow::onRoutingProfiles);
    connect(m_geoDataManagerAction, &QAction::triggered, this, &MainWindow::onGeoDataManager);
    connect(m_ruleSetManagerAction, &QAction::triggered, this, &MainWindow::onRuleSetManager);
    connect(m_dnsProfilesAction, &QAction::triggered, this, &MainWindow::onDnsProfiles);
    connect(m_previewSingBoxTunConfigAction, &QAction::triggered, this,
            &MainWindow::onPreviewSingBoxTunConfig);
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
    connect(&m_appController, &AppController::logLine, this, &MainWindow::appendLog);
    if (HelperProcessManager* helper = m_appController.helperProcessManager()) {
        connect(helper, &HelperProcessManager::killSwitchStateChanged, this,
                [this](const KillSwitchState&) { updateStatusBar(); });
        connect(helper, &HelperProcessManager::tunExitedWithKillSwitchActive, this,
                [this]() { updateStatusBar(); });
    }
    connect(&m_appController, &AppController::coreStateChanged, this, [this]() {
        updateStatusBar();
        if (m_trayController) {
            m_trayController->updateMenuState();
        }
    });
    connect(&m_appController, &AppController::proxyStateChanged, this, [this]() {
        updateStatusBar();
        if (m_trayController) {
            m_trayController->updateMenuState();
        }
    });
    connect(&m_appController, &AppController::quitApproved, this, &MainWindow::onQuitApproved);

    connect(m_testSelectedAction, &QAction::triggered, this, &MainWindow::onTestSelected);
    connect(m_testAllAction, &QAction::triggered, this, &MainWindow::onTestAll);
    connect(m_testTcpSelectedAction, &QAction::triggered, this, &MainWindow::onTestTcpSelected);
    connect(m_testDelaySelectedAction, &QAction::triggered, this, &MainWindow::onTestDelaySelected);
    connect(m_cancelTestsAction, &QAction::triggered, this, &MainWindow::onCancelTests);

    connect(&m_testManager, &TestManager::testStarted, this, &MainWindow::onTestStarted);
    connect(&m_testManager, &TestManager::profileUpdated, this, &MainWindow::onProfileUpdated);
    connect(&m_testManager, &TestManager::progressChanged, this,
            &MainWindow::onTestProgressChanged);
    connect(&m_testManager, &TestManager::allFinished, this, &MainWindow::onAllTestsFinished);
    connect(&m_testManager, &TestManager::logLine, this, &MainWindow::appendLog);
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

QString MainWindow::routingStatusText() const
{
    return m_routingManager.activeProfile().name;
}

QString MainWindow::dnsStatusText() const
{
    return m_dnsManager.activeProfile().name;
}

QString MainWindow::runtimeStatusText() const
{
    if (m_coreManager.isRunning()) {
        return runtimeModeDisplayString(m_appController.activeRuntimeMode());
    }
    return runtimeModeDisplayString(AppSettings::instance().effectiveRuntimeMode());
}

QString MainWindow::killSwitchStatusText() const
{
    HelperProcessManager* helper = m_appController.helperProcessManager();
    if (!helper) {
        return QStringLiteral("off");
    }
    const KillSwitchState state = helper->killSwitchState();
    switch (state.status) {
    case KillSwitchStatus::Enabled:
        return QStringLiteral("on");
    case KillSwitchStatus::Failed:
        return QStringLiteral("failed");
    case KillSwitchStatus::NeedsRecovery:
        return QStringLiteral("needs recovery");
    case KillSwitchStatus::Unsupported:
        return QStringLiteral("unsupported");
    case KillSwitchStatus::Enabling:
    case KillSwitchStatus::Disabling:
        return killSwitchStatusToString(state.status);
    case KillSwitchStatus::Disabled:
        break;
    }
    return QStringLiteral("off");
}

void MainWindow::checkKillSwitchRecoveryOnStartup()
{
    if (QFile::exists(AppPaths::killSwitchMarkerPath())) {
        appendLog(QStringLiteral("Kill switch recovery marker detected at startup."));
    }

    HelperProcessManager* helper = m_appController.helperProcessManager();
    if (!helper) {
        return;
    }

    QString error;
    if (!helper->connectToHelper(&error)) {
        if (QFile::exists(AppPaths::killSwitchMarkerPath())) {
            QMessageBox box(this);
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle(QStringLiteral("Kill switch recovery"));
            box.setText(QStringLiteral(
                "Zarya detected a previous kill switch state. Network access may be restricted.\n\n"
                "Helper is not running. Start helper manually (elevated on Linux) and use "
                "Disable Kill Switch, or follow recovery instructions."));
            QPushButton* recoveryButton =
                box.addButton(QStringLiteral("Show Recovery Instructions"),
                              QMessageBox::ActionRole);
            box.addButton(QStringLiteral("Ignore"), QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == recoveryButton) {
                QMessageBox::information(this, QStringLiteral("Kill switch recovery"),
                                         HelperProcessManager::recoveryInstructionsText());
            }
        }
        return;
    }

    helper->killSwitchStatus(&error);
    if (helper->killSwitchState().status != KillSwitchStatus::NeedsRecovery
        && !QFile::exists(AppPaths::killSwitchMarkerPath())) {
        return;
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Kill switch recovery"));
    box.setText(QStringLiteral(
        "Zarya detected a previous kill switch state. Network access may be restricted."));
    QPushButton* disableButton =
        box.addButton(QStringLiteral("Disable Kill Switch"), QMessageBox::AcceptRole);
    QPushButton* recoveryButton =
        box.addButton(QStringLiteral("Show Recovery Instructions"), QMessageBox::ActionRole);
    box.addButton(QStringLiteral("Ignore"), QMessageBox::RejectRole);
    box.setDefaultButton(disableButton);
    box.exec();

    if (box.clickedButton() == disableButton) {
        QString disableError;
        if (!helper->killSwitchDisable(&disableError)) {
            QMessageBox::warning(this, QStringLiteral("Kill switch"), disableError);
        } else {
            appendLog(QStringLiteral("Kill switch disabled during startup recovery."));
        }
    } else if (box.clickedButton() == recoveryButton) {
        QMessageBox::information(this, QStringLiteral("Kill switch recovery"),
                                 HelperProcessManager::recoveryInstructionsText());
    }
}

void MainWindow::checkUncleanTunShutdownWarning()
{
    if (!AppSettings::instance().shouldWarnUncleanTunShutdown()) {
        return;
    }
    appendLog(QStringLiteral(
        "Previous session may have exited while experimental TUN mode was active."));
    QMessageBox::warning(
        this, QStringLiteral("TUN recovery"),
        QStringLiteral(
            "Zarya may have exited unexpectedly while TUN mode was active.\n\n"
            "If networking is broken, stop remaining sing-box processes or reboot.\n\n"
            "Automatic route recovery is not implemented yet."));
}

void MainWindow::setupAppController()
{
    m_appController.setDialogParent(this);
    m_appController.setAfterCoreStartedCallback([this]() {
        tryAutoEnableSystemProxy(m_appController.lastStartWasAutostart());
    });
    m_appController.setSaveApplicationStateCallback([this](QString* error) {
        saveWindowState();
        return saveAll(error);
    });
    m_appController.setOpenGeoDataManagerCallback([this]() { onGeoDataManager(); });
    m_appController.setOpenDnsProfilesCallback([this]() { onDnsProfiles(); });
    m_appController.setOpenRuleSetManagerCallback([this]() { onRuleSetManager(); });
}

void MainWindow::setupTray()
{
    m_trayController = new TrayController(&m_appController, this, this);
    if (m_trayController->isAvailable()) {
        m_hideToTrayAction->setEnabled(true);
    } else {
        m_hideToTrayAction->setEnabled(false);
        appendLog(QStringLiteral("Tray unavailable"));
    }
}

AppController* MainWindow::appController()
{
    return &m_appController;
}

bool MainWindow::hasSelectedProfile() const
{
    return selectedRow() >= 0;
}

bool MainWindow::isTestingBusy() const
{
    return m_testManager.isBusy();
}

bool MainWindow::isSubscriptionUpdateBusy() const
{
    return m_subscriptionUpdateBusy;
}

bool MainWindow::canRestoreSystemProxy() const
{
    return m_systemProxy.hasSavedState() && m_systemProxy.isSupported();
}

bool MainWindow::trayIsAvailable() const
{
    return m_trayController && m_trayController->isAvailable();
}

void MainWindow::showFromTray()
{
    show();
    raise();
    activateWindow();
    appendLog(QStringLiteral("Window restored from tray"));
}

void MainWindow::hideToTray(bool notify)
{
    if (!trayIsAvailable()) {
        hide();
        return;
    }
    hide();
    appendLog(QStringLiteral("Window hidden to tray"));
    if (notify && AppSettings::instance().showTrayNotifications() && !m_trayCloseNotificationShown) {
        notifyTray(QStringLiteral("Zarya"),
                   QStringLiteral("Zarya is still running in the system tray."));
        m_trayCloseNotificationShown = true;
    }
}

void MainWindow::startSelectedProfile()
{
    Profile* profilePtr = selectedProfileInStorage();
    if (!profilePtr) {
        appendLog(QStringLiteral("Select a profile to start."));
        return;
    }
    m_appController.startProfile(*profilePtr);
}

void MainWindow::testSelected()
{
    onTestSelected();
}

void MainWindow::updateAllSubscriptions()
{
    onUpdateAllSubscriptions();
}

void MainWindow::requestApplicationQuit()
{
    m_appController.requestQuit();
}

bool MainWindow::shouldHideToTrayOnClose() const
{
    return trayIsAvailable() && AppSettings::instance().minimizeToTrayOnClose();
}

void MainWindow::notifyTray(const QString& title, const QString& message)
{
    if (m_trayController) {
        m_trayController->showNotification(title, message);
    }
}

QString MainWindow::trayStatusText() const
{
    return trayIsAvailable() ? QStringLiteral("available") : QStringLiteral("unavailable");
}

void MainWindow::saveWindowState()
{
    QSettings& settings = AppSettings::settings();
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
    if (m_splitter) {
        settings.setValue(QStringLiteral("window/splitter"), m_splitter->saveState());
    }
    const int row = selectedRow();
    if (row >= 0) {
        settings.setValue(QStringLiteral("window/lastProfileId"),
                          m_tableModel.profileAt(row).id);
    }
}

void MainWindow::restoreWindowState()
{
    QSettings& settings = AppSettings::settings();
    if (settings.contains(QStringLiteral("window/geometry"))) {
        restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    }
    if (settings.contains(QStringLiteral("window/state"))) {
        restoreState(settings.value(QStringLiteral("window/state")).toByteArray());
    }
    if (m_splitter && settings.contains(QStringLiteral("window/splitter"))) {
        m_splitter->restoreState(settings.value(QStringLiteral("window/splitter")).toByteArray());
    }
    const QString lastProfileId = settings.value(QStringLiteral("window/lastProfileId")).toString();
    if (!lastProfileId.isEmpty()) {
        selectProfileById(lastProfileId);
    }
}

void MainWindow::selectProfileById(const QString& profileId)
{
    for (int row = 0; row < m_tableModel.rowCount(); ++row) {
        if (m_tableModel.profileAt(row).id == profileId) {
            m_tableView->selectRow(row);
            break;
        }
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange && trayIsAvailable()
        && AppSettings::instance().minimizeToTrayOnMinimize()
        && (windowState() & Qt::WindowMinimized)) {
        event->accept();
        hideToTray(false);
        return;
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::updateStatusBar()
{
    if (m_testManager.isBusy()) {
        statusBar()->showMessage(
            QStringLiteral(
                "Testing: %1/%2 | Core: %3 | Runtime: %4 | Kill switch: %5 | System proxy: %6 | "
                "Routing: %7 | DNS: %8")
                .arg(m_testProgressDone)
                .arg(m_testProgressTotal)
                .arg(coreStatusText())
                .arg(runtimeStatusText())
                .arg(killSwitchStatusText())
                .arg(systemProxyStatusText())
                .arg(routingStatusText())
                .arg(dnsStatusText()));
        return;
    }

    const AppSettings& settings = AppSettings::instance();
    QString message =
        QStringLiteral(
            "Idle | Core: %1 | Runtime: %2 | Kill switch: %3 | System proxy: %4 | Routing: %5 | "
            "DNS: %6 | Tray: %7")
            .arg(coreStatusText(), runtimeStatusText(), killSwitchStatusText(),
                 systemProxyStatusText(), routingStatusText(), dnsStatusText(), trayStatusText());

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

void MainWindow::logStartupContext(const StartupOptions& options)
{
    if (AppPaths::isPortableMode()) {
        appendLog(QStringLiteral("Portable mode enabled"));
    }
    appendLog(QStringLiteral("Data dir: %1").arg(AppPaths::dataDir()));
    appendLog(QStringLiteral("Runtime dir: %1").arg(AppPaths::runtimeDir()));
    appendLog(QStringLiteral("Packaging: %1 %2")
                  .arg(PackagingInfo::versionString(), PackagingInfo::platformName()));
    appendLog(QStringLiteral("Log level: %1")
                  .arg(StartupOptionsParser::logLevelToString(options.logLevel)));
}

void MainWindow::finishStartup(const StartupOptions& options)
{
    checkKillSwitchRecoveryOnStartup();

    const AppSettings& settings = AppSettings::instance();
    const bool startMinimized =
        options.startMinimizedEffective(settings.startMinimizedToTray());

    if (startMinimized) {
        if (trayIsAvailable()) {
            appendLog(QStringLiteral("Starting minimized to tray"));
            hideToTray(false);
        } else {
            appendLog(QStringLiteral("Tray unavailable; showing main window"));
            show();
        }
    } else {
        show();
    }

    QString profileIdToStart;
    if (!options.startProfileId.isEmpty()) {
        profileIdToStart = options.startProfileId;
    } else if (!options.noAutostartProfile && settings.autoStartLastProfile()) {
        profileIdToStart = settings.lastStartedProfileId();
    }

    if (profileIdToStart.isEmpty()) {
        return;
    }

    const int delayMs = settings.autoStartDelaySeconds() * 1000;
    appendLog(QStringLiteral("Scheduling profile autostart in %1 seconds")
                  .arg(settings.autoStartDelaySeconds()));

    QTimer::singleShot(delayMs, this, [this, profileIdToStart]() {
        if (!startProfileById(profileIdToStart, true)) {
            notifyTray(QStringLiteral("Zarya"),
                       QStringLiteral("Autostart profile failed. See log for details."));
        }
    });
}

bool MainWindow::startProfileById(const QString& profileId, bool fromAutostart)
{
    Profile* profile = profileById(profileId);
    if (!profile) {
        appendLog(QStringLiteral("Autostart profile failed: profile not found (%1)").arg(profileId));
        return false;
    }
    if (!profile->enabled) {
        appendLog(QStringLiteral("Autostart profile failed: profile is disabled (%1)")
                      .arg(profile->name));
        return false;
    }

    appendLog(QStringLiteral("Autostarting profile: %1").arg(profile->name));
    selectProfileById(profileId);
    return m_appController.startProfile(*profile, fromAutostart);
}

Profile* MainWindow::profileById(const QString& profileId)
{
    for (Profile& profile : m_allProfiles) {
        if (profile.id == profileId) {
            return &profile;
        }
    }
    return nullptr;
}

void MainWindow::tryAutoEnableSystemProxy(bool fromAutostart)
{
    if (AppSettings::instance().effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental) {
        return;
    }
    const AppSettings& settings = AppSettings::instance();
    const bool shouldEnable = fromAutostart ? settings.autoEnableSystemProxyAfterAutoStart()
                                            : settings.autoEnableSystemProxyOnStart();
    if (!shouldEnable) {
        return;
    }

    if (m_systemProxy.supportLevel() == QStringLiteral("partial")) {
        appendLog(QStringLiteral(
            "Auto system proxy was requested, but this desktop is not supported yet."));
        appendLog(m_systemProxy.limitations());
        return;
    }

    if (!m_systemProxy.isSupported()) {
        appendLog(QStringLiteral("Auto system proxy was requested, but system proxy is unsupported."));
        if (!m_systemProxy.limitations().isEmpty()) {
            appendLog(m_systemProxy.limitations());
        }
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
    if (m_quitting) {
        QMainWindow::closeEvent(event);
        return;
    }

    if (shouldHideToTrayOnClose()) {
        event->ignore();
        hideToTray(true);
        return;
    }

    event->ignore();
    requestApplicationQuit();
}

void MainWindow::onQuitApproved()
{
    m_quitting = true;
    QApplication::quit();
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

    m_routingManager.load();
    m_dnsManager.load();

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
    if (!m_routingManager.save(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    if (!m_dnsManager.save(&error)) {
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
    m_subscriptionUpdateBusy = true;
    if (m_trayController) {
        m_trayController->updateMenuState();
    }
    const QVector<SubscriptionUpdateResult> results =
        m_subscriptionManager.updateAll(m_subscriptions, m_allProfiles);
    m_subscriptionUpdateBusy = false;
    if (m_trayController) {
        m_trayController->updateMenuState();
    }
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
    SettingsDialog dialog(m_routingManager, m_dnsManager, m_appController.helperProcessManager(),
                        this);
    dialog.exec();
    appendLog(QStringLiteral("Settings updated. Xray path: %1")
                  .arg(AppSettings::instance().resolvedXrayPath()));
    updateStatusBar();
}

void MainWindow::onRuleSetManager()
{
    RuleSetManagerDialog dialog(m_ruleSetManager, m_routingManager, m_dnsManager,
                                [this](const QString& line) { appendLog(line); }, this);
    dialog.exec();
}

void MainWindow::onGeoDataManager()
{
    GeoDataManagerDialog dialog(m_geoDataManager, [this](const QString& line) { appendLog(line); },
                                this);
    dialog.exec();
}

void MainWindow::onDnsProfiles()
{
    DnsManagerDialog dialog(m_dnsManager, [this](const QString& line) { appendLog(line); }, this);
    connect(&dialog, &DnsManagerDialog::activeProfileChanged, this,
            [this](const QString& name) {
                Q_UNUSED(name);
                updateStatusBar();
            });
    dialog.exec();
    QString error;
    m_dnsManager.save(&error);
    updateStatusBar();
}

void MainWindow::onPreviewSingBoxTunConfig()
{
    Profile* profile = selectedProfileInStorage();
    if (!profile) {
        QMessageBox::information(this, QStringLiteral("Preview sing-box config"),
                                 QStringLiteral("Select a profile first."));
        return;
    }

    const SingBoxConfigGenerationResult generation = m_appController.generateSingBoxTunConfig(*profile);
    if (!generation.success) {
        QMessageBox::warning(this, QStringLiteral("Preview sing-box config"),
                             generation.errorMessage);
        return;
    }

    const QString json =
        QString::fromUtf8(QJsonDocument(generation.config).toJson(QJsonDocument::Indented));
    SingBoxConfigPreviewDialog dialog(json, generation.warnings, &m_coreManager, this);
    dialog.exec();
}

void MainWindow::checkGeoDataOnStartup()
{
    if (!GeoDataSettingsStore::instance().autoCheckOnStartup()) {
        return;
    }
    appendLog(QStringLiteral("Checking geo data status"));
    const QVector<GeoDataFileStatus> statuses = m_geoDataManager.checkAllStatus();
    for (const GeoDataFileStatus& status : statuses) {
        if (status.status == GeoDataStatus::Missing) {
            appendLog(QStringLiteral("%1 missing").arg(status.fileName));
        } else if (status.status == GeoDataStatus::NotWritable) {
            appendLog(QStringLiteral("%1: %2").arg(status.fileName, status.error));
        } else {
            appendLog(QStringLiteral("%1 present, size %2 bytes")
                          .arg(status.fileName)
                          .arg(status.sizeBytes));
        }
    }
}

void MainWindow::onRoutingProfiles()
{
    RoutingManagerDialog dialog(m_routingManager, [this](const QString& line) { appendLog(line); },
                              this);
    connect(&dialog, &RoutingManagerDialog::activeProfileChanged, this,
            [this](const QString& name) {
                Q_UNUSED(name);
                updateStatusBar();
            });
    dialog.exec();
    QString error;
    m_routingManager.save(&error);
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
    startSelectedProfile();
}

void MainWindow::onStopCore()
{
    m_appController.stopCurrentProfile();
}

void MainWindow::onEnableSystemProxy()
{
    if (!m_coreManager.isRunning()) {
        QMessageBox::information(
            this, QStringLiteral("System proxy"),
            QStringLiteral("Core is not running. Start a profile before enabling system proxy."));
        return;
    }

    if (m_systemProxy.supportLevel() == QStringLiteral("partial")
        || !m_systemProxy.isSupported()) {
        QMessageBox::information(
            this, QStringLiteral("System proxy"),
            m_systemProxy.limitations().isEmpty()
                ? QStringLiteral("System proxy is not supported on this platform.")
                : m_systemProxy.limitations());
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
    notifyTray(QStringLiteral("Zarya"), QStringLiteral("Proxy core started"));
    updateStatusBar();
}

void MainWindow::onCoreStopped()
{
    appendLog(QStringLiteral("Core stopped."));
    notifyTray(QStringLiteral("Zarya"), QStringLiteral("Proxy core stopped"));
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

QVector<QString> MainWindow::collectSelectedProfileIds() const
{
    QVector<QString> ids;
    const QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    ids.reserve(selected.size());
    for (const QModelIndex& index : selected) {
        if (!index.isValid()) {
            continue;
        }
        ids.append(m_tableModel.profileAt(index.row()).id);
    }
    return ids;
}

QVector<QString> MainWindow::collectAllTestableProfileIds() const
{
    QVector<QString> ids;
    for (const Profile& profile : m_allProfiles) {
        if (!profile.enabled || profile.deletedBySubscriptionUpdate) {
            continue;
        }
        ids.append(profile.id);
    }
    return ids;
}

void MainWindow::startTestsForIds(const QVector<QString>& profileIds, TestMode mode)
{
    if (profileIds.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Test"),
                                 QStringLiteral("Select at least one profile to test."));
        return;
    }

    QVector<Profile> profiles;
    profiles.reserve(profileIds.size());
    for (const QString& id : profileIds) {
        const int index = indexOfProfileById(id);
        if (index >= 0) {
            profiles.append(m_allProfiles.at(index));
        }
    }

    if (profiles.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Test"),
                                 QStringLiteral("No matching profiles found."));
        return;
    }

    setTestingUiBusy(true);
    m_testProgressDone = 0;
    m_testProgressTotal = profiles.size();
    updateStatusBar();
    m_testManager.startTests(profiles, mode);
}

void MainWindow::setTestingUiBusy(bool busy)
{
    m_cancelTestsAction->setEnabled(busy);
    m_testSelectedAction->setEnabled(!busy);
    m_testAllAction->setEnabled(!busy);
    m_testTcpSelectedAction->setEnabled(!busy);
    m_testDelaySelectedAction->setEnabled(!busy);
}

void MainWindow::showProfileContextMenu(const QPoint& position)
{
    const QModelIndex index = m_tableView->indexAt(position);
    if (!index.isValid()) {
        return;
    }
    m_tableView->selectionModel()->select(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QMenu menu(this);
    menu.addAction(QStringLiteral("Test this profile"), this, &MainWindow::onTestProfileContext);
    menu.addAction(QStringLiteral("Test TCP"), this, &MainWindow::onTestTcpContext);
    menu.addAction(QStringLiteral("Test real delay"), this, &MainWindow::onTestDelayContext);
    menu.exec(m_tableView->viewport()->mapToGlobal(position));
}

void MainWindow::onTestSelected()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::TcpThenRealDelay);
}

void MainWindow::onTestAll()
{
    startTestsForIds(collectAllTestableProfileIds(), TestMode::TcpThenRealDelay);
}

void MainWindow::onTestTcpSelected()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::TcpOnly);
}

void MainWindow::onTestDelaySelected()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::RealDelayOnly);
}

void MainWindow::onCancelTests()
{
    m_testManager.cancel();
}

void MainWindow::onTestProfileContext()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::TcpThenRealDelay);
}

void MainWindow::onTestTcpContext()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::TcpOnly);
}

void MainWindow::onTestDelayContext()
{
    startTestsForIds(collectSelectedProfileIds(), TestMode::RealDelayOnly);
}

void MainWindow::onTestStarted(const QString& profileId)
{
    const int index = indexOfProfileById(profileId);
    if (index < 0) {
        return;
    }
    Profile& profile = m_allProfiles[index];
    profile.lastTestStatus = TestStatus::Testing;
    profile.lastTestError.clear();
    refreshProfileView();
}

void MainWindow::onProfileUpdated(const Profile& profile)
{
    const int index = indexOfProfileById(profile.id);
    if (index < 0) {
        return;
    }
    m_allProfiles[index] = profile;
    refreshProfileView();
    QString error;
    if (!m_profileStore.save(m_allProfiles, &error) && !error.isEmpty()) {
        appendLog(QStringLiteral("Failed to save test results: %1").arg(error));
    }
}

void MainWindow::onTestProgressChanged(int done, int total)
{
    m_testProgressDone = done;
    m_testProgressTotal = total;
    updateStatusBar();
}

void MainWindow::onAllTestsFinished()
{
    setTestingUiBusy(false);
    m_testProgressDone = m_testProgressTotal;
    updateStatusBar();
    appendLog(QStringLiteral("All tests finished."));
}

void MainWindow::onAbout()
{
    QMessageBox::about(
        this, QStringLiteral("About Zarya"),
        QStringLiteral(
            "Zarya 0.7\n\nNative proxy profile manager with system tray, Xray multi-protocol "
            "support, Windows system proxy, subscriptions, and node testing."));
}

} // namespace zarya
