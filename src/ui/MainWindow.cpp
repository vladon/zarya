#include "ui/MainWindow.h"

#include "ui/TrayController.h"

#include "domain/CoreType.h"
#include "domain/Profile.h"
#include "domain/ProtocolType.h"
#include "domain/Subscription.h"
#include "app/Application.h"
#include "app/StartupOptions.h"
#include "errors/AppError.h"
#include "errors/ErrorCode.h"
#include "errors/ErrorPresenter.h"
#include "migration/MigrationManager.h"
#include "app/BuildInfo.h"
#include "packaging/PackagingInfo.h"
#include "packaging/InstallationMode.h"
#include "packaging/PortableMigration.h"
#include "packaging/PublicBetaDocs.h"
#include "diagnostics/SupportSummary.h"
#include "recovery/StartupRecovery.h"
#include "storage/SettingsValidator.h"
#include "ui/BetaBannerWidget.h"
#include "ui/onboarding/FirstRunState.h"
#include "ui/onboarding/FirstRunWizard.h"
#include "ui/StartupRecoveryDialog.h"
#include "ui/widgets/StatusDashboardWidget.h"
#include "errors/ErrorCode.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "helperclient/HelperProcessManager.h"
#include "service/HelperServiceManagerFactory.h"
#include "killswitch/KillSwitchState.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "ui/ImportVlessDialog.h"
#include "ui/ProfileDialog.h"
#include "ui/DnsManagerDialog.h"
#include "diagnostics/DiagnosticsContext.h"
#include "logging/LogBuffer.h"
#include "ui/BackupExportDialog.h"
#include "ui/DiagnosticsDialog.h"
#include "ui/BackupImportDialog.h"
#include "ui/CoreManagerDialog.h"
#include "ui/GeoDataManagerDialog.h"
#include "ui/RuleSetManagerDialog.h"
#include "ui/RoutingManagerDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/AppUpdateDialog.h"
#include "updater/AppUpdateChecker.h"
#include "updater/AppUpdateStateManager.h"
#include "features/FeatureGate.h"
#include "features/FeaturePolicy.h"
#include "updater/AppUpdatePlanner.h"
#include "geodata/GeoDataFileStatus.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/GeoDataSettingsStore.h"
#include "ui/SubscriptionManagerDialog.h"
#include "ui/SingBoxConfigPreviewDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QJsonDocument>
#include <QCloseEvent>
#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QScrollArea>
#include <QEvent>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
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
#include <QDateTime>

namespace zarya {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_appController(&m_coreManager, &m_systemProxy, &m_xrayAdapter, &m_testManager,
                      &m_routingManager, &m_geoDataManager, &m_dnsManager, &m_ruleSetManager,
                      this)
{
    LogBuffer::instance().setAppStartedAt(QDateTime::currentDateTimeUtc());
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupAppController();
    setupConnections();
    setupTray();
    m_coreBinaryManager.setProcessCoreManager(&m_coreManager);
    m_coreBinaryManager.setSingBoxRunningCallback([this]() {
        return m_appController.activeRuntimeMode() == RuntimeMode::TunSingBoxExperimental
               && m_appController.isCoreRunning();
    });
    connect(&m_coreBinaryManager, &CoreBinaryManager::logLine, this, &MainWindow::appendLog);
    connect(&m_subscriptionManager, &SubscriptionManager::logLine, this, &MainWindow::appendLog);

    const StartupOptions& startupOptions = Application::instance()->startupOptions();
    runPreLoadStartup(startupOptions);
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
    runStartupRecovery();
    appendLog(QStringLiteral("Subscriptions: %1").arg(m_subscriptionStore.filePath()));
    appendLog(QStringLiteral("Xray path: %1").arg(AppSettings::instance().resolvedXrayPath()));
    if (!m_systemProxy.isSupported()) {
        appendLog(QStringLiteral("System proxy unsupported on this platform."));
    }
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Zarya"));
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
    m_logView->setPlaceholderText(tr("Core and application logs appear here…"));

    m_emptyStateLabel = new QLabel(this);
    m_emptyStateLabel->setWordWrap(true);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setStyleSheet(QStringLiteral("padding:12px; color:#555;"));
    m_emptyStateLabel->hide();

    m_logFilterCombo = new QComboBox(this);
    m_logFilterCombo->addItem(tr("All"), QStringLiteral("all"));
    m_logFilterCombo->addItem(tr("Errors"), QStringLiteral("errors"));
    m_logFilterCombo->addItem(tr("Warnings"), QStringLiteral("warnings"));
    m_logFilterCombo->addItem(tr("Runtime"), QStringLiteral("runtime"));
    m_logFilterCombo->addItem(tr("Subscriptions"), QStringLiteral("subscriptions"));
    m_logFilterCombo->addItem(tr("Core"), QStringLiteral("core"));
    m_logFilterCombo->addItem(tr("Helper"), QStringLiteral("helper"));
    m_logFilterCombo->addItem(tr("Kill Switch"), QStringLiteral("killswitch"));
    connect(m_logFilterCombo, &QComboBox::currentIndexChanged, this, [this]() {
        m_logFilterKey = m_logFilterCombo->currentData().toString();
        m_logView->clear();
        for (const QString& line : LogBuffer::instance().recentLines(5000)) {
            if (lineMatchesLogFilter(line)) {
                m_logView->appendPlainText(line);
            }
        }
    });

    auto* logToolbar = new QWidget(this);
    auto* copySelectedBtn = new QPushButton(tr("Copy selected"), logToolbar);
    auto* copyAllBtn = new QPushButton(tr("Copy all visible"), logToolbar);
    auto* clearViewBtn = new QPushButton(tr("Clear view"), logToolbar);
    auto* diagBtn = new QPushButton(tr("Create diagnostics"), logToolbar);
    connect(copySelectedBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_logView->textCursor().selectedText());
    });
    connect(copyAllBtn, &QPushButton::clicked, this,
            [this]() { QApplication::clipboard()->setText(m_logView->toPlainText()); });
    connect(clearViewBtn, &QPushButton::clicked, this, [this]() { m_logView->clear(); });
    connect(diagBtn, &QPushButton::clicked, this, &MainWindow::onCreateDiagnosticsBundle);
    auto* logToolbarLayout = new QHBoxLayout(logToolbar);
    logToolbarLayout->setContentsMargins(0, 0, 0, 0);
    logToolbarLayout->addWidget(new QLabel(tr("Log filter:"), logToolbar));
    logToolbarLayout->addWidget(m_logFilterCombo);
    logToolbarLayout->addWidget(copySelectedBtn);
    logToolbarLayout->addWidget(copyAllBtn);
    logToolbarLayout->addWidget(clearViewBtn);
    logToolbarLayout->addWidget(diagBtn);
    logToolbarLayout->addStretch();

    auto* logPanel = new QWidget(this);
    auto* logPanelLayout = new QVBoxLayout(logPanel);
    logPanelLayout->setContentsMargins(0, 0, 0, 0);
    logPanelLayout->addWidget(logToolbar);
    logPanelLayout->addWidget(m_logView);

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->addWidget(m_tableView);
    m_splitter->addWidget(logPanel);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);

    m_statusDashboard = new StatusDashboardWidget(this);
    connect(m_statusDashboard, &StatusDashboardWidget::openCoreManagerRequested, this,
            &MainWindow::onCoreManager);
    connect(m_statusDashboard, &StatusDashboardWidget::addProfileRequested, this,
            &MainWindow::onAddProfile);
    connect(m_statusDashboard, &StatusDashboardWidget::addSubscriptionRequested, this,
            &MainWindow::onSubscriptions);
    connect(m_statusDashboard, &StatusDashboardWidget::runSetupRequested, this,
            [this]() { runFirstRunWizard(true); });
    connect(m_statusDashboard, &StatusDashboardWidget::pasteLinkRequested, this,
            &MainWindow::onImportVless);
    connect(m_statusDashboard, &StatusDashboardWidget::importBackupRequested, this,
            &MainWindow::onImportBackup);
    connect(m_statusDashboard, &StatusDashboardWidget::startRequested, this,
            &MainWindow::onStartCore);
    connect(m_statusDashboard, &StatusDashboardWidget::stopRequested, this,
            &MainWindow::onStopCore);
    connect(m_statusDashboard, &StatusDashboardWidget::testRequested, this,
            &MainWindow::onTestSelected);
    connect(m_statusDashboard, &StatusDashboardWidget::updateSubscriptionsRequested, this,
            &MainWindow::onUpdateAllSubscriptions);
    connect(m_statusDashboard, &StatusDashboardWidget::openLogsRequested, this, [this]() {
        m_splitter->setSizes({height() / 3, height() * 2 / 3});
        m_logView->setFocus();
    });
    connect(m_statusDashboard, &StatusDashboardWidget::createDiagnosticsRequested, this,
            &MainWindow::onCreateDiagnosticsBundle);

    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(4, 4, 4, 4);
    if (PackagingInfo::isPreReleaseBannerBuild() && !AppSettings::instance().dismissBetaBanner()) {
        m_betaBanner = new BetaBannerWidget(central);
        centralLayout->addWidget(m_betaBanner);
    }
    centralLayout->addWidget(m_statusDashboard);
    centralLayout->addWidget(m_emptyStateLabel);
    centralLayout->addWidget(m_splitter, 1);
    setCentralWidget(central);

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    m_saveAction = fileMenu->addAction(tr("&Save profiles"));
    m_loadAction = fileMenu->addAction(tr("&Reload profiles"));
    fileMenu->addSeparator();
    m_exportBackupAction = fileMenu->addAction(tr("Export &Backup…"));
    m_importBackupAction = fileMenu->addAction(tr("Import &Backup…"));
    fileMenu->addAction(tr("Import from Portable Zarya &Folder…"), this,
                        &MainWindow::onImportFromPortableFolder);
    fileMenu->addSeparator();
    m_settingsAction = fileMenu->addAction(tr("&Settings…"));
    fileMenu->addSeparator();
    m_showAction = fileMenu->addAction(tr("&Show"));
    m_hideToTrayAction = fileMenu->addAction(tr("Hide to &Tray"));
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction(tr("E&xit"));
    connect(m_showAction, &QAction::triggered, this, &MainWindow::showFromTray);
    connect(m_hideToTrayAction, &QAction::triggered, this, [this]() { hideToTray(false); });
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::requestApplicationQuit);

    auto* profileMenu = menuBar()->addMenu(tr("&Profiles"));
    m_addAction = profileMenu->addAction(tr("&Add…"));
    m_editAction = profileMenu->addAction(tr("&Edit…"));
    m_deleteAction = profileMenu->addAction(tr("&Delete"));
    profileMenu->addSeparator();
    m_importAction = profileMenu->addAction(tr("Import Profile &Links…"));
    fileMenu->insertAction(m_settingsAction, m_importAction);
    fileMenu->insertSeparator(m_settingsAction);

    auto* subscriptionsMenu = menuBar()->addMenu(tr("Su&bscriptions"));
    m_subscriptionsAction = subscriptionsMenu->addAction(tr("&Manage…"));
    m_updateSubscriptionAction =
        subscriptionsMenu->addAction(tr("Update &Selected"));
    m_updateAllSubscriptionsAction =
        subscriptionsMenu->addAction(tr("Update &All"));

    auto* testMenu = menuBar()->addMenu(tr("&Test"));
    m_testSelectedAction = testMenu->addAction(tr("Test &Selected"));
    m_testAllAction = testMenu->addAction(tr("Test &All"));
    testMenu->addSeparator();
    m_testTcpSelectedAction = testMenu->addAction(tr("Test &TCP Selected"));
    m_testDelaySelectedAction = testMenu->addAction(tr("Test &Delay Selected"));
    testMenu->addSeparator();
    m_cancelTestsAction = testMenu->addAction(tr("&Cancel Tests"));
    m_cancelTestsAction->setEnabled(false);

    auto* coreMenu = menuBar()->addMenu(tr("&Core"));
    m_startAction = coreMenu->addAction(tr("&Start"));
    m_stopAction = coreMenu->addAction(tr("S&top"));
    m_stopAction->setEnabled(false);

    auto* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_routingProfilesAction =
        toolsMenu->addAction(tr("Routing &Profiles…"));
    m_geoDataManagerAction = toolsMenu->addAction(tr("Geo Data &Manager…"));
    m_coreManagerAction = toolsMenu->addAction(tr("&Core Manager…"));
    m_ruleSetManagerAction = toolsMenu->addAction(tr("sing-box Rule &Sets…"));
    m_dnsProfilesAction = toolsMenu->addAction(tr("DNS &Profiles…"));
    m_previewSingBoxTunConfigAction =
        toolsMenu->addAction(tr("Preview sing-box TUN config…"));
    const bool experimentalVisible = FeatureGate::showExperimentalFeatures();
    m_ruleSetManagerAction->setVisible(experimentalVisible);
    m_previewSingBoxTunConfigAction->setVisible(experimentalVisible);
    toolsMenu->addSeparator();
    m_enableSystemProxyAction =
        toolsMenu->addAction(tr("Enable &System Proxy"));
    m_restoreSystemProxyAction =
        toolsMenu->addAction(tr("&Restore Previous Proxy"));

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("Public Beta &Guide"), this, [this]() {
        if (!PublicBetaDocs::openBundledDoc(QStringLiteral("README.md"))) {
            QMessageBox::warning(this, tr("Help"),
                                 tr("Public beta guide is not bundled with this build."));
        }
    });
    helpMenu->addAction(tr("&Quick Start"), this, [this]() {
        if (!PublicBetaDocs::openBundledDoc(QStringLiteral("quick-start.md"))) {
            QMessageBox::warning(this, tr("Help"),
                                 tr("Quick start guide is not bundled with this build."));
        }
    });
    helpMenu->addAction(tr("&Known Limitations"), this, [this]() {
        if (!PublicBetaDocs::openBundledDoc(QStringLiteral("known-limitations.md"))) {
            QMessageBox::warning(this, tr("Help"),
                                 tr("Known limitations doc is not bundled with this build."));
        }
    });
    helpMenu->addAction(tr("&Report Issue…"), this, [this]() {
        if (!PublicBetaDocs::openIssueReporting()) {
            QMessageBox::warning(this, tr("Help"),
                                 tr("Issue reporting instructions are not bundled with this build."));
        }
    });
    helpMenu->addSeparator();
    helpMenu->addAction(tr("Run Setup &Wizard…"), this,
                        [this]() { runFirstRunWizard(true); });
    helpMenu->addAction(tr("Create &Diagnostics Bundle…"), this,
                        &MainWindow::onCreateDiagnosticsBundle);
    helpMenu->addAction(tr("Copy &Support Summary"), this, &MainWindow::onCopySupportSummary);
    helpMenu->addAction(tr("Check for App &Updates…"), this, &MainWindow::onCheckAppUpdates);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About"), this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar(tr("Main"));
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
    connect(m_exportBackupAction, &QAction::triggered, this, &MainWindow::onExportBackup);
    connect(m_importBackupAction, &QAction::triggered, this, &MainWindow::onImportBackup);
    connect(&m_backupManager, &BackupManager::logLine, this, &MainWindow::appendLog);
    connect(&m_diagnosticsManager, &DiagnosticsManager::logLine, this, &MainWindow::appendLog);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    connect(m_routingProfilesAction, &QAction::triggered, this, &MainWindow::onRoutingProfiles);
    connect(m_geoDataManagerAction, &QAction::triggered, this, &MainWindow::onGeoDataManager);
    connect(m_coreManagerAction, &QAction::triggered, this, &MainWindow::onCoreManager);
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
        connect(helper, &HelperProcessManager::helperLogLine, this,
                [this](const QString& line) {
                    appendLog(QStringLiteral("helper: %1").arg(line));
                });
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
    LogBuffer::instance().append(line);
    if (lineMatchesLogFilter(line)) {
        m_logView->appendPlainText(line);
    }
}

bool MainWindow::lineMatchesLogFilter(const QString& line) const
{
    if (m_logFilterKey.isEmpty() || m_logFilterKey == QStringLiteral("all")) {
        return true;
    }
    const QString lower = line.toLower();
    if (m_logFilterKey == QStringLiteral("errors")) {
        return lower.contains(QStringLiteral("error")) || lower.contains(QStringLiteral("failed"));
    }
    if (m_logFilterKey == QStringLiteral("warnings")) {
        return lower.contains(QStringLiteral("warning"));
    }
    if (m_logFilterKey == QStringLiteral("runtime")) {
        return lower.contains(QStringLiteral("xray")) || lower.contains(QStringLiteral("sing-box"))
               || lower.contains(QStringLiteral("starting")) || lower.contains(QStringLiteral("stopping"))
               || lower.contains(QStringLiteral("runtime"));
    }
    if (m_logFilterKey == QStringLiteral("subscriptions")) {
        return lower.contains(QStringLiteral("subscription"));
    }
    if (m_logFilterKey == QStringLiteral("core")) {
        return lower.contains(QStringLiteral("core")) || lower.contains(QStringLiteral("xray"));
    }
    if (m_logFilterKey == QStringLiteral("helper")) {
        return lower.contains(QStringLiteral("helper"));
    }
    if (m_logFilterKey == QStringLiteral("killswitch")) {
        return lower.contains(QStringLiteral("kill switch")) || lower.contains(QStringLiteral("killswitch"));
    }
    return true;
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
            box.setWindowTitle(tr("Kill switch recovery"));
            box.setText(tr(
                "Zarya detected a previous kill switch state. Network access may be restricted.\n\n"
                "Helper is not running. Start helper manually (elevated on Linux) and use "
                "Disable Kill Switch, or follow recovery instructions."));
            QPushButton* recoveryButton =
                box.addButton(tr("Show Recovery Instructions"),
                              QMessageBox::ActionRole);
            box.addButton(tr("Ignore"), QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == recoveryButton) {
                QMessageBox::information(this, tr("Kill switch recovery"),
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
    box.setWindowTitle(tr("Kill switch recovery"));
    box.setText(tr(
        "Zarya detected a previous kill switch state. Network access may be restricted."));
    QPushButton* disableButton =
        box.addButton(tr("Disable Kill Switch"), QMessageBox::AcceptRole);
    QPushButton* recoveryButton =
        box.addButton(tr("Show Recovery Instructions"), QMessageBox::ActionRole);
    box.addButton(tr("Ignore"), QMessageBox::RejectRole);
    box.setDefaultButton(disableButton);
    box.exec();

    if (box.clickedButton() == disableButton) {
        QString disableError;
        if (!helper->killSwitchDisable(&disableError)) {
            QMessageBox::warning(this, tr("Kill switch"), disableError);
        } else {
            appendLog(QStringLiteral("Kill switch disabled during startup recovery."));
        }
    } else if (box.clickedButton() == recoveryButton) {
        QMessageBox::information(this, tr("Kill switch recovery"),
                                 HelperProcessManager::recoveryInstructionsText());
    }
}

void MainWindow::runPreLoadStartup(const StartupOptions& options)
{
    const MigrationResult migration = MigrationManager::runStartupMigrations();
    for (const QString& line : migration.logLines) {
        appendLog(line);
    }
    if (!migration.ok) {
        AppError error = appErrorFromCode(ErrorCode::migrationFailed(),
                                          migration.errors.join(QStringLiteral("\n")));
        error.details = migration.errors.join(QStringLiteral("\n"));
        ErrorPresenter::show(this, error, true);
    }

    const SettingsValidationResult validation = SettingsValidator::validateAndFixOnStartup();
    for (const QString& fix : validation.autoFixed) {
        appendLog(QStringLiteral("Settings: %1").arg(fix));
    }
    for (const QString& warning : validation.warnings) {
        appendLog(QStringLiteral("Settings warning: %1").arg(warning));
    }

    LogLevel effectiveLevel = AppSettings::instance().logLevel();
    if (options.logLevel != LogLevel::Info) {
        effectiveLevel = options.logLevel;
        AppSettings::instance().setLogLevel(effectiveLevel);
    }
    LogBuffer::instance().setMinLogLevel(effectiveLevel);
}

void MainWindow::runStartupRecovery()
{
    const StartupRecoveryPlan plan = StartupRecovery::detect();
    if (!plan.uncleanShutdown && plan.detectedLines.isEmpty()) {
        return;
    }

    appendLog(QStringLiteral("Unclean previous shutdown detected."));
    StartupRecoveryDialog dialog(plan, this);
    connect(&dialog, &StartupRecoveryDialog::createDiagnosticsRequested, this,
            &MainWindow::onCreateDiagnosticsBundle);
    connect(&dialog, &StartupRecoveryDialog::openReportingGuideRequested, this, []() {
        PublicBetaDocs::openIssueReporting();
    });
    if (dialog.exec() != QDialog::Accepted) {
        appendLog(QStringLiteral("Startup recovery skipped by user."));
        return;
    }

    QStringList logLines;
    const StartupRecoveryPlan selected = dialog.selectedPlan();
    if (!StartupRecovery::apply(selected, &logLines)) {
        appendLog(QStringLiteral("Startup recovery encountered errors."));
    }
    for (const QString& line : logLines) {
        appendLog(line);
    }
}

void MainWindow::maybeShowFirstRunWizard()
{
    m_coreBinaryManager.refreshLocalState();
    if (!FirstRunState::shouldShowWizard(m_coreBinaryManager, m_allProfiles.size(),
                                         m_subscriptions.size())) {
        return;
    }
    runFirstRunWizard(false);
}

void MainWindow::runFirstRunWizard(bool force)
{
    Q_UNUSED(force);
    FirstRunWizard wizard(&m_coreBinaryManager, &m_routingManager, &m_dnsManager, this);
    connect(&wizard, &FirstRunWizard::openCoreManagerRequested, this, &MainWindow::onCoreManager);
    connect(&wizard, &FirstRunWizard::chooseXrayBinaryRequested, this,
            [this]() { chooseCoreBinary(CoreType::Xray); });
    connect(&wizard, &FirstRunWizard::chooseSingBoxBinaryRequested, this,
            [this]() { chooseCoreBinary(CoreType::SingBox); });
    connect(&wizard, &FirstRunWizard::installXrayRequested, this, [this]() {
        onCoreManager();
        m_coreBinaryManager.updateCore(CoreType::Xray);
    });
    connect(&wizard, &FirstRunWizard::installSingBoxRequested, this, [this]() {
        onCoreManager();
        m_coreBinaryManager.updateCore(CoreType::SingBox);
    });
    connect(&wizard, &FirstRunWizard::openRoutingProfilesRequested, this,
            &MainWindow::onRoutingProfiles);
    connect(&wizard, &FirstRunWizard::openDnsProfilesRequested, this, &MainWindow::onDnsProfiles);
    connect(&wizard, &FirstRunWizard::importBackupRequested, this, &MainWindow::onImportBackup);
    connect(&wizard, &FirstRunWizard::addProfileManuallyRequested, this, &MainWindow::onAddProfile);
    connect(&wizard, &FirstRunWizard::configureHelperRequested, this, &MainWindow::onSettings);
    connect(&wizard, &FirstRunWizard::wizardFinishedState, this, &MainWindow::applyFirstRunState);

    if (wizard.exec() == QDialog::Accepted) {
        AppSettings::instance().setFirstRunCoreInstalled(
            m_coreBinaryManager.infoFor(CoreType::Xray).exists);
        AppSettings::instance().setFirstRunProfilesImported(
            !m_allProfiles.isEmpty() || !m_subscriptions.isEmpty());
        AppSettings::instance().setFirstRunCompleted(true);
    } else if (wizard.wasSkipped()) {
        AppSettings::instance().setFirstRunCompleted(true);
        appendLog(QStringLiteral("First-run setup skipped."));
    }
    updateStatusDashboard();
    updateEmptyState();
}

void MainWindow::applyFirstRunState(const FirstRunState& state)
{
    if (!state.importedProfiles.isEmpty()) {
        for (const Profile& profile : state.importedProfiles) {
            m_allProfiles.append(profile);
        }
        saveAll();
        refreshProfileView();
    }
    if (!state.importedProfiles.isEmpty() || !state.addedSubscriptions.isEmpty()) {
        AppSettings::instance().setFirstRunProfilesImported(true);
    }
    if (!state.addedSubscriptions.isEmpty()) {
        const int beforeCount = m_subscriptions.size();
        for (const Subscription& sub : state.addedSubscriptions) {
            m_subscriptions.append(sub);
        }
        m_subscriptionStore.save(m_subscriptions);
        for (int index = beforeCount; index < m_subscriptions.size(); ++index) {
            m_subscriptionManager.updateSubscription(m_subscriptions[index], m_allProfiles);
        }
        m_subscriptionStore.save(m_subscriptions);
        saveAll();
        refreshProfileView();
    }
    m_routingManager.setActiveProfileId(state.routingProfileId);
    m_dnsManager.setActiveProfileId(state.dnsProfileId);
    m_routingManager.save();
    m_dnsManager.save();

    AppSettings& settings = AppSettings::instance();
    settings.setRuntimeMode(state.runtimeMode);
    if (state.runtimeMode == RuntimeMode::TunSingBoxExperimental) {
        settings.setEnableExperimentalTun(true);
        settings.setTunWarningAccepted(state.tunWarningAccepted);
    }

    if (!m_allProfiles.isEmpty()) {
        selectProfileById(m_allProfiles.first().id);
    }
    if (state.startProfileOnFinish && !m_allProfiles.isEmpty()) {
        startSelectedProfile();
    }
}

void MainWindow::updateStatusDashboard()
{
    if (!m_statusDashboard) {
        return;
    }
    m_coreBinaryManager.refreshLocalState();
    const CoreInfo xray = m_coreBinaryManager.infoFor(CoreType::Xray);
    const bool hasProfiles = !m_allProfiles.isEmpty();
    const bool configured = xray.exists && hasProfiles;

    StatusDashboardModel model;
    model.configured = configured;
    model.running = m_appController.isCoreRunning();
    model.routingText = m_routingManager.activeProfile().name;
    model.dnsText = m_dnsManager.activeProfile().name;
    model.systemProxyText =
        m_systemProxy.enabledByZarya() ? QStringLiteral("On") : QStringLiteral("Off");
    model.coreText = xray.exists ? QStringLiteral("Xray %1").arg(xray.installedVersion) : QStringLiteral("missing");
    const AppSettings& settings = AppSettings::instance();
    model.httpEndpoint = QStringLiteral("127.0.0.1:%1").arg(settings.httpPort());
    model.socksEndpoint = QStringLiteral("127.0.0.1:%1").arg(settings.socksPort());

    Profile* selected = selectedProfileInStorage();
    if (!selected && !m_allProfiles.isEmpty()) {
        selected = &m_allProfiles.first();
    }
    model.profileName = selected ? selected->name : QString();
    model.runtimeText = runtimeStatusText();
    model.recommendedRuntimeText = tr("Xray system proxy");
    model.experimentalRuntimeActive = FeatureGate::experimentalRuntimeEffective();

    m_statusDashboard->updateModel(model);
}

void MainWindow::updateEmptyState()
{
    if (!m_emptyStateLabel) {
        return;
    }
    const int visibleCount = m_tableModel.rowCount();
    if (visibleCount > 0) {
        m_emptyStateLabel->hide();
        return;
    }
    m_emptyStateLabel->show();
    if (!m_subscriptions.isEmpty()) {
        m_emptyStateLabel->setText(
            tr("Subscriptions exist but no profiles are imported yet.\n\n"
               "Use Subscriptions → Update All to import profiles."));
    } else if (!m_profileFilterKey.isEmpty() && m_profileFilterKey != QStringLiteral("__manual__")
               && !m_allProfiles.isEmpty()) {
        m_emptyStateLabel->setText(
            tr("No profiles match the current filter.\n\nClear the filter to see all profiles."));
    } else {
        m_emptyStateLabel->setText(
            tr("No profiles yet.\n\nAdd a proxy profile manually, paste a share link, "
               "or add a subscription."));
    }
}

Profile* MainWindow::resolveProfileForStart()
{
    Profile* profile = selectedProfileInStorage();
    if (profile) {
        return profile;
    }
    QVector<Profile*> enabled;
    for (Profile& p : m_allProfiles) {
        if (p.enabled && !p.deletedBySubscriptionUpdate) {
            enabled.append(&p);
        }
    }
    if (enabled.size() == 1) {
        selectProfileById(enabled.first()->id);
        return enabled.first();
    }
    if (enabled.isEmpty()) {
        return nullptr;
    }
    QStringList names;
    for (Profile* p : enabled) {
        names.append(p->name);
    }
    bool ok = false;
    const QString chosen =
        QInputDialog::getItem(this, tr("Select profile"), tr("Profile:"),
                              names, 0, false, &ok);
    if (!ok) {
        return nullptr;
    }
    for (Profile* p : enabled) {
        if (p->name == chosen) {
            selectProfileById(p->id);
            return p;
        }
    }
    return nullptr;
}

void MainWindow::handleErrorAction(ErrorAction action, const AppError& error)
{
    Q_UNUSED(error);
    switch (action) {
    case ErrorAction::OpenCoreManager:
        onCoreManager();
        break;
    case ErrorAction::ChooseExistingBinary:
        chooseCoreBinary(CoreType::Xray);
        break;
    case ErrorAction::CreateDiagnostics:
        onCreateDiagnosticsBundle();
        break;
    case ErrorAction::OpenSettings:
        onSettings();
        break;
    case ErrorAction::OpenRuleSetManager:
        onRuleSetManager();
        break;
    case ErrorAction::OpenGeoDataManager:
        onGeoDataManager();
        break;
    case ErrorAction::SwitchToSystemProxy:
        AppSettings::instance().setRuntimeMode(RuntimeMode::SystemProxyXray);
        AppSettings::instance().setEnableExperimentalTun(false);
        break;
    case ErrorAction::OpenProfile:
        onEditProfile();
        break;
    default:
        break;
    }
}

void MainWindow::setupAppController()
{
    m_helperServiceManager = HelperServiceManagerFactory::create();
    if (HelperProcessManager* helper = m_appController.helperProcessManager()) {
        helper->setServiceManager(m_helperServiceManager.get());
    }

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
    m_coreBinaryManager.refreshLocalState();
    const CoreInfo xray = m_coreBinaryManager.infoFor(CoreType::Xray);
    if (!xray.exists) {
        AppError error = appErrorFromCode(ErrorCode::coreXrayMissing());
        const ErrorAction action = ErrorPresenter::showWithActions(this, error);
        handleErrorAction(action, error);
        return;
    }

    Profile* profilePtr = resolveProfileForStart();
    if (!profilePtr) {
        appendLog(QStringLiteral("No profile available to start."));
        updateEmptyState();
        return;
    }

    const AppSettings& settings = AppSettings::instance();
    if (settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental) {
        QString unsupportedReason;
        const SingBoxConfigGenerator generator;
        if (!generator.supportsProfile(*profilePtr, &unsupportedReason)) {
            AppError error = appErrorFromCode(ErrorCode::profileUnsupportedRuntime(), unsupportedReason);
            error.message = tr("This profile is not supported by the current runtime.");
            const ErrorAction action = ErrorPresenter::showWithActions(this, error);
            handleErrorAction(action, error);
            return;
        }
    } else if (!m_xrayAdapter.supportsProfile(*profilePtr, nullptr)) {
        AppError error = appErrorFromCode(ErrorCode::profileUnsupportedRuntime());
        error.message = tr("This profile is not supported by the current runtime.");
        const ErrorAction action = ErrorPresenter::showWithActions(this, error);
        handleErrorAction(action, error);
        return;
    }

    m_appController.startProfile(*profilePtr);
    updateStatusDashboard();
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
    updateStatusDashboard();

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
               this, tr("Change system proxy"),
               tr("Zarya will change Windows system proxy settings. Continue?"))
           == QMessageBox::Yes;
}

void MainWindow::logStartupContext(const StartupOptions& options)
{
    if (AppPaths::isPortableMode()) {
        appendLog(QStringLiteral("Portable mode enabled"));
    }
    appendLog(QStringLiteral("Data dir: %1").arg(AppPaths::dataDir()));
    appendLog(QStringLiteral("Runtime dir: %1").arg(AppPaths::runtimeDir()));
    appendLog(QStringLiteral("Zarya %1 (%2) commit %3 built %4 Qt %5")
                  .arg(BuildInfo::appVersion(),
                       BuildInfo::buildChannel(),
                       BuildInfo::buildCommit(),
                       BuildInfo::buildDateUtc(),
                       BuildInfo::qtVersion()));
    appendLog(QStringLiteral("Log level: %1")
                  .arg(StartupOptionsParser::logLevelToString(options.logLevel)));
}

void MainWindow::finishStartup(const StartupOptions& options)
{
    checkKillSwitchRecoveryOnStartup();

    if (options.updateRollbackNotice) {
        AppUpdateStartupNotice notice;
        notice.kind = AppUpdateStartupNotice::Kind::Failed;
        AppUpdateStateManager::showStartupNotice(this, notice);
    } else if (options.postUpdateNotice) {
        AppUpdateStartupNotice notice;
        notice.kind = AppUpdateStartupNotice::Kind::Success;
        AppUpdateStateManager::showStartupNotice(this, notice);
    } else {
        const AppUpdateStartupNotice notice = AppUpdateStateManager::checkStartupState();
        if (notice.kind != AppUpdateStartupNotice::Kind::None) {
            AppUpdateStateManager::showStartupNotice(this, notice);
        }
    }

    maybeShowFirstRunWizard();
    maybeShowInstalledPortableImportPrompt();
    checkCoreUpdatesOnStartup();
    checkAppUpdatesOnStartup();
    warnIfExperimentalRuntimeDisabledOnStartup();

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
        QMessageBox::warning(this, tr("System proxy"),
                             tr("Failed to restore system proxy:\n%1")
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
    m_profileFilterCombo->addItem(tr("All profiles"), QString());
    m_profileFilterCombo->addItem(tr("Manual"), QStringLiteral("__manual__"));
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
    updateEmptyState();
    updateStatusDashboard();
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
            this, tr("Update subscription"),
            tr("Select a subscription in the profile filter, or use Subscriptions → "
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
        QMessageBox::warning(this, tr("Update subscription"),
                             tr("Subscription not found."));
        return;
    }

    const SubscriptionUpdateResult result =
        m_subscriptionManager.updateSubscription(m_subscriptions[index], m_allProfiles);
    QString error;
    if (!saveAll(&error)) {
        QMessageBox::warning(this, tr("Save failed"), error);
    }
    refreshProfileFilterCombo();
    refreshProfileView();

    if (!result.success) {
        QMessageBox::warning(this, tr("Update failed"), result.errorMessage);
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
        QMessageBox::warning(this, tr("Save failed"), error);
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
        QMessageBox::warning(this, tr("Update all"),
                             tr("%1 subscription(s) failed to update.").arg(failed));
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
        QMessageBox::information(this, tr("Edit profile"),
                                 tr("Select a profile first."));
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
        QMessageBox::information(this, tr("Delete profile"),
                                 tr("Select a profile first."));
        return;
    }

    const QString profileId = m_tableModel.profileAt(row).id;
    const int index = indexOfProfileById(profileId);
    if (index < 0) {
        return;
    }
    const QString name = m_allProfiles.at(index).name;
    if (QMessageBox::question(this, tr("Delete profile"),
                              tr("Delete profile \"%1\"?").arg(name))
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
        QMessageBox::warning(this, tr("Save failed"), error);
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
        QMessageBox::warning(this, tr("Load failed"), error);
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
                          m_helperServiceManager.get(), this);
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

void MainWindow::onCoreManager()
{
    CoreManagerDialog dialog(m_coreBinaryManager,
                             [this](const QString& line) { appendLog(line); }, this);
    dialog.exec();
    m_coreBinaryManager.refreshLocalState();
}

void MainWindow::onExportBackup()
{
    QString error;
    if (!saveAll(&error)) {
        QMessageBox::warning(this, tr("Export Backup"),
                             tr("Could not save current state before export:\n%1")
                                 .arg(error));
        return;
    }

    BackupExportDialog dialog(m_backupManager,
                              [this](const QString& line) { appendLog(line); }, this);
    dialog.exec();
}

void MainWindow::onImportFromPortableFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Import from Portable Zarya Folder"));
    if (folder.isEmpty()) {
        return;
    }

    const PortableDataPreview preview = PortableMigration::preview(folder);
    if (!preview.valid) {
        QMessageBox::warning(
            this, tr("Portable Import"),
            tr("The selected folder does not look like a portable Zarya install.\n\n"
               "Expected portable.flag or a data/ folder with profiles, subscriptions, or "
               "settings."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Portable Import"),
        tr("Portable Zarya data found:\n\n"
           "Profiles: %1\n"
           "Subscriptions: %2\n"
           "Routing: %3\n"
           "DNS: %4\n"
           "Settings: %5\n\n"
           "A temporary backup archive will be created and opened in the import flow.\n"
           "The original portable folder will not be modified.\n\n"
           "Continue?")
            .arg(preview.profileCount)
            .arg(preview.subscriptionCount)
            .arg(preview.hasRouting ? tr("yes") : tr("no"))
            .arg(preview.hasDns ? tr("yes") : tr("no"))
            .arg(preview.hasSettings ? tr("yes") : tr("no")));
    if (answer != QMessageBox::Yes) {
        return;
    }

    const QString tempArchive =
        QDir(QDir::tempPath())
            .filePath(QStringLiteral("zarya-portable-import-%1.zarya-backup.zip")
                          .arg(QDateTime::currentDateTime().toString(
                              QStringLiteral("yyyyMMdd-HHmmss"))));
    QString error;
    if (!PortableMigration::createBackupArchive(folder, tempArchive, &error)) {
        QMessageBox::critical(this, tr("Portable Import"), error);
        return;
    }

    const bool coreRunning = m_appController.isCoreRunning();
    HelperProcessManager* helper = m_appController.helperProcessManager();
    const bool killSwitchActive = BackupManager::isKillSwitchActive(helper);
    if (killSwitchActive) {
        QMessageBox::warning(this, tr("Portable Import"),
                             BackupManager::runtimeBlockReason(coreRunning, killSwitchActive));
        return;
    }
    if (coreRunning) {
        const auto proceed = QMessageBox::question(
            this, tr("Portable Import"),
            tr("A proxy core is currently running. Import is disabled until the core is stopped.\n\n"
               "Open import dialog anyway?"));
        if (proceed != QMessageBox::Yes) {
            return;
        }
    }

    BackupImportDialog dialog(m_backupManager, coreRunning, killSwitchActive,
                              [this](const QString& line) { appendLog(line); }, this,
                              tempArchive);
    if (dialog.exec() != QDialog::Accepted || !dialog.importApplied()) {
        return;
    }

    loadAllOnStartup();
    m_ruleSetManager.reload();
    appendLog(QStringLiteral("Configuration reloaded after portable folder import."));
}

void MainWindow::onImportBackup()
{
    const bool coreRunning = m_appController.isCoreRunning();
    HelperProcessManager* helper = m_appController.helperProcessManager();
    const bool killSwitchActive = BackupManager::isKillSwitchActive(helper);

    if (killSwitchActive || coreRunning) {
        const QString reason =
            BackupManager::runtimeBlockReason(coreRunning, killSwitchActive);
        QMessageBox::warning(this, tr("Import Backup"), reason);
        if (killSwitchActive) {
            return;
        }
    }

    if (coreRunning) {
        const auto answer = QMessageBox::question(
            this, tr("Import Backup"),
            tr("A proxy core is currently running. You can preview a backup, but "
               "import is disabled until the core is stopped.\n\nOpen import dialog "
               "anyway?"));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    BackupImportDialog dialog(m_backupManager, coreRunning, killSwitchActive,
                              [this](const QString& line) { appendLog(line); }, this);
    if (dialog.exec() != QDialog::Accepted || !dialog.importApplied()) {
        return;
    }

    loadAllOnStartup();
    m_ruleSetManager.reload();
    appendLog(QStringLiteral("Configuration reloaded after backup import."));
}

DiagnosticsContext MainWindow::buildDiagnosticsContext()
{
    DiagnosticsContext context;
    context.profiles = m_allProfiles;
    context.subscriptions = m_subscriptions;
    if (Profile* selected = selectedProfileInStorage()) {
        context.selectedProfile = *selected;
        context.hasSelectedProfile = true;
    }
    context.coreManager = &m_coreManager;
    context.coreBinaryManager = &m_coreBinaryManager;
    context.appController = &m_appController;
    context.systemProxy = &m_systemProxy;
    context.routingManager = &m_routingManager;
    context.dnsManager = &m_dnsManager;
    context.geoDataManager = &m_geoDataManager;
    context.ruleSetManager = &m_ruleSetManager;
    context.helper = m_appController.helperProcessManager();
    context.helperService = m_helperServiceManager.get();
    context.xrayAdapter = &m_xrayAdapter;
    context.appStartedAt = LogBuffer::instance().appStartedAt();
    context.systemTrayAvailable = trayIsAvailable();
    return context;
}

void MainWindow::onCreateDiagnosticsBundle()
{
    m_diagnosticsManager.setContext(buildDiagnosticsContext());
    DiagnosticsDialog dialog(m_diagnosticsManager,
                             [this](const QString& line) { appendLog(line); }, this);
    dialog.exec();
}

void MainWindow::onCopySupportSummary()
{
    if (!SupportSummary::copyToClipboard(buildDiagnosticsContext())) {
        QMessageBox::warning(this, tr("Support Summary"),
                             tr("Could not copy support summary to the clipboard."));
        return;
    }
    QMessageBox::information(
        this, tr("Support Summary"),
        tr("Support summary copied to the clipboard.\n\n"
           "Review it before pasting into an issue. Do not include proxy links or passwords."));
}

void MainWindow::checkCoreUpdatesOnStartup()
{
    if (!AppSettings::instance().checkCoreUpdatesOnStartup()) {
        return;
    }
    appendLog(QStringLiteral("Checking core updates on startup"));
    m_coreBinaryManager.refreshLocalState();
    m_coreBinaryManager.checkLatestVersions();
}

void MainWindow::onCheckAppUpdates()
{
    AppUpdateDialog dialog(&m_appController, [this]() { return isTestingBusy(); }, this);
    dialog.exec();
}

void MainWindow::maybeShowInstalledPortableImportPrompt()
{
    if (InstallationInfo::detect() != InstallationMode::Installed) {
        return;
    }
    AppSettings& settings = AppSettings::instance();
    if (settings.installedPortableImportPromptShown()) {
        return;
    }
    if (!m_allProfiles.isEmpty() || !m_subscriptions.isEmpty()) {
        settings.setInstalledPortableImportPromptShown(true);
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Portable Zarya data"),
        tr("Have a portable Zarya folder with profiles or subscriptions?\n\n"
           "You can import data from a portable Zarya folder without modifying the original "
           "folder.\n\n"
           "Import now?"),
        QMessageBox::Yes | QMessageBox::No);
    settings.setInstalledPortableImportPromptShown(true);
    if (answer == QMessageBox::Yes) {
        onImportFromPortableFolder();
    }
}

void MainWindow::warnIfExperimentalRuntimeDisabledOnStartup()
{
    const AppSettings& settings = AppSettings::instance();
    if (settings.configuredRuntimeMode() != RuntimeMode::TunSingBoxExperimental
        || !settings.enableExperimentalTun()) {
        return;
    }
    if (settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental) {
        return;
    }
    const ReleaseChannel channel =
        FeaturePolicy::releaseChannelFromString(settings.releaseChannelKey());
    const bool isStableLike = PackagingInfo::isStableBuild()
                              || PackagingInfo::isReleaseCandidateBuild()
                              || FeaturePolicy::isStableLikeChannel(channel);
    const QString logLine =
        isStableLike
            ? QStringLiteral("Experimental TUN mode is disabled in stable builds by default. "
                             "Effective runtime: Xray system proxy.")
            : QStringLiteral("Experimental runtime is disabled. "
                             "Effective runtime: Xray system proxy.");
    appendLog(logLine);
    const QString dialogText =
        isStableLike
            ? tr("Experimental TUN mode is disabled in stable builds by default.\n"
                 "Effective runtime: Xray system proxy.\n\n"
                 "Enable experimental features in Settings → Release channel if you intend to "
                 "use TUN.")
            : tr("Experimental runtime is disabled.\n"
                 "Effective runtime: Xray system proxy.\n\n"
                 "Enable experimental features in Settings → Release channel if you intend to "
                 "use TUN.");
    QMessageBox::warning(this, tr("Experimental runtime disabled"), dialogText);
}

void MainWindow::checkAppUpdatesOnStartup()
{
    if (!AppSettings::instance().checkAppUpdatesOnStartup()) {
        return;
    }
    if (AppSettings::instance().appUpdateManifestUrl().trimmed().isEmpty()) {
        return;
    }
    appendLog(QStringLiteral("Checking app updates on startup"));
    auto* checker = new AppUpdateChecker(this);
    connect(checker, &AppUpdateChecker::updateCheckFinished, this,
            [this, checker](const AppUpdatePlan& plan) {
                if (plan.updateAvailable) {
                    appendLog(QStringLiteral("App update available: %1").arg(plan.latestVersion));
                } else {
                    appendLog(QStringLiteral("App update check: already on latest channel version"));
                }
                checker->deleteLater();
            });
    connect(checker, &AppUpdateChecker::updateCheckFailed, this,
            [this, checker](const QString& error) {
                appendLog(QStringLiteral("App update check failed: %1").arg(error));
                checker->deleteLater();
            });
    checker->checkForUpdates();
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
        QMessageBox::information(this, tr("Preview sing-box config"),
                                 tr("Select a profile first."));
        return;
    }

    const SingBoxConfigGenerationResult generation = m_appController.generateSingBoxTunConfig(*profile);
    if (!generation.success) {
        QMessageBox::warning(this, tr("Preview sing-box config"),
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
            this, tr("System proxy"),
            tr("Core is not running. Start a profile before enabling system proxy."));
        return;
    }

    if (m_systemProxy.supportLevel() == QStringLiteral("partial")
        || !m_systemProxy.isSupported()) {
        QMessageBox::information(
            this, tr("System proxy"),
            m_systemProxy.limitations().isEmpty()
                ? tr("System proxy is not supported on this platform.")
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
        QMessageBox::warning(this, tr("System proxy"),
                             tr("Failed to enable system proxy:\n%1").arg(error));
    }
    updateStatusBar();
}

void MainWindow::onRestoreSystemProxy()
{
    if (!m_systemProxy.hasSavedState()) {
        QMessageBox::information(
            this, tr("System proxy"),
            tr("No saved previous proxy state. Nothing to restore."));
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
    QMessageBox::warning(this, tr("Core error"), message);
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
        QMessageBox::information(this, tr("Test"),
                                 tr("Select at least one profile to test."));
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
        QMessageBox::information(this, tr("Test"),
                                 tr("No matching profiles found."));
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
    menu.addAction(tr("Test this profile"), this, &MainWindow::onTestProfileContext);
    menu.addAction(tr("Test TCP"), this, &MainWindow::onTestTcpContext);
    menu.addAction(tr("Test real delay"), this, &MainWindow::onTestDelayContext);
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
    QMessageBox::about(this, tr("About Zarya"), BuildInfo::aboutText());
}

bool MainWindow::chooseCoreBinary(CoreType coreType)
{
    const QString title = coreType == CoreType::SingBox ? tr("Select sing-box executable")
                                                        : tr("Select Xray executable");
    const QString path = QFileDialog::getOpenFileName(
        this, title, {},
        QStringLiteral("Executables (*.exe);;All files (*.*)"));
    if (path.isEmpty()) {
        return false;
    }

    AppSettings& settings = AppSettings::instance();
    if (coreType == CoreType::SingBox) {
        settings.setSingBoxExecutablePath(path);
    } else {
        settings.setXrayExecutablePath(path);
    }
    m_coreBinaryManager.refreshLocalState();
    updateStatusDashboard();
    return true;
}

} // namespace zarya
