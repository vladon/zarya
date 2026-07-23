#include "ui/AppUpdateDialog.h"

#include "app/AppController.h"
#include "app/BuildInfo.h"
#include "cores/CoreDownloader.h"
#include "killswitch/KillSwitchMarker.h"
#include "packaging/InstallationMode.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"
#include "updater/AppUpdateChannel.h"
#include "updater/AppUpdatePaths.h"
#include "updater/AppUpdateStager.h"
#include "updater/AppUpdateStatus.h"
#include "updater/AppUpdateVerifier.h"
#include "updater/installers/LinuxUpdateInstaller.h"
#include "updater/installers/MacosUpdateInstaller.h"
#include "updater/installers/PortableUpdateInstaller.h"
#include "updater/installers/WindowsInstalledUpdateInstaller.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

AppUpdateDialog::AppUpdateDialog(AppController* controller,
                                 const std::function<bool()>& isTestsRunning,
                                 QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_isTestsRunning(isTestsRunning)
{
    setWindowTitle(tr("Zarya App Updates"));
    resize(640, 560);

    m_currentVersionLabel = new QLabel(this);
    m_channelLabel = new QLabel(this);
    m_installationModeLabel = new QLabel(this);
    m_manifestLabel = new QLabel(this);
    m_manifestLabel->setWordWrap(true);
    m_statusLabel = new QLabel(tr("No update checked yet."), this);
    m_statusLabel->setWordWrap(true);

    m_detailsText = new QPlainTextEdit(this);
    m_detailsText->setReadOnly(true);

    m_checkButton = new QPushButton(tr("Check Now"), this);
    m_chooseManifestButton = new QPushButton(tr("Choose Local Manifest…"), this);
    m_downloadButton = new QPushButton(tr("Download and Verify"), this);
    m_installButton = new QPushButton(tr("Install and Restart"), this);
    m_openDownloadsButton = new QPushButton(tr("Open Downloads Folder"), this);
    auto* cancelButton = new QPushButton(tr("Cancel"), this);

    m_downloadButton->setEnabled(false);
    m_installButton->setEnabled(false);

    connect(m_checkButton, &QPushButton::clicked, this, &AppUpdateDialog::onCheckNow);
    connect(m_chooseManifestButton, &QPushButton::clicked, this,
            &AppUpdateDialog::onChooseLocalManifest);
    connect(m_downloadButton, &QPushButton::clicked, this, &AppUpdateDialog::onDownloadAndVerify);
    connect(m_installButton, &QPushButton::clicked, this, &AppUpdateDialog::onInstallAndRestart);
    connect(m_openDownloadsButton, &QPushButton::clicked, this,
            &AppUpdateDialog::onOpenDownloadsFolder);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    connect(&m_checker, &AppUpdateChecker::updateCheckStarted, this, &AppUpdateDialog::onCheckStarted);
    connect(&m_checker, &AppUpdateChecker::updateCheckFinished, this,
            &AppUpdateDialog::onCheckFinished);
    connect(&m_checker, &AppUpdateChecker::updateCheckFailed, this, &AppUpdateDialog::onCheckFailed);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_checkButton);
    buttonRow->addWidget(m_chooseManifestButton);
    buttonRow->addWidget(m_downloadButton);
    buttonRow->addWidget(m_installButton);
    buttonRow->addWidget(m_openDownloadsButton);
    buttonRow->addStretch();
    buttonRow->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("<b>Current version</b>"), this));
    layout->addWidget(m_currentVersionLabel);
    layout->addWidget(new QLabel(tr("<b>Channel</b>"), this));
    layout->addWidget(m_channelLabel);
    layout->addWidget(new QLabel(tr("<b>Installation mode</b>"), this));
    layout->addWidget(m_installationModeLabel);
    layout->addWidget(new QLabel(tr("<b>Manifest</b>"), this));
    layout->addWidget(m_manifestLabel);
    layout->addWidget(new QLabel(tr("<b>Status</b>"), this));
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_detailsText, 1);
    layout->addLayout(buttonRow);

    refreshStaticInfo();
}

void AppUpdateDialog::refreshStaticInfo()
{
    const AppSettings& settings = AppSettings::instance();
    m_currentVersionLabel->setText(BuildInfo::appVersion());
    m_channelLabel->setText(settings.appUpdateChannelKey());
    m_installationModeLabel->setText(InstallationInfo::currentModeString());

    QString manifestText;
    if (!m_checker.localManifestPath().isEmpty()) {
        manifestText = tr("Local file: %1").arg(m_checker.localManifestPath());
    } else if (settings.appUpdateManifestUrl().trimmed().isEmpty()) {
        manifestText = tr("Not configured");
    } else {
        manifestText = tr("Configured URL");
    }
    m_manifestLabel->setText(manifestText);
}

void AppUpdateDialog::setStatusText(const QString& text)
{
    m_statusLabel->setText(text);
}

bool AppUpdateDialog::killSwitchActive() const
{
    return KillSwitchMarker::exists();
}

void AppUpdateDialog::setBusy(bool busy)
{
    m_checkButton->setEnabled(!busy);
    m_chooseManifestButton->setEnabled(!busy);
    m_downloadButton->setEnabled(!busy && m_hasPlan && m_lastPlan.updateAvailable);
    m_openDownloadsButton->setEnabled(!busy);
    refreshInstallButtonState();
}

void AppUpdateDialog::refreshInstallButtonState()
{
    if (m_controller == nullptr) {
        m_installButton->setEnabled(false);
        return;
    }

    QString reason;
    const bool testsRunning = m_isTestsRunning ? m_isTestsRunning() : false;

    const bool canInstall = m_hasPlan
                            && PortableUpdateInstaller::canInstallPortableUpdate(
                                m_lastPlan.selectedAsset, m_artifactVerified, m_stagingReady,
                                m_controller->isCoreRunning(), testsRunning, killSwitchActive(),
                                &reason);
    m_installButton->setEnabled(canInstall);
    if (!canInstall && !reason.isEmpty() && m_artifactVerified) {
        m_installButton->setToolTip(reason);
    } else {
        m_installButton->setToolTip({});
    }
}

QString AppUpdateDialog::installStatusMessage() const
{
#if defined(Q_OS_WIN)
    if (InstallationInfo::currentMode() == InstallationMode::Installed) {
        return WindowsInstalledUpdateInstaller::statusMessage();
    }
#elif defined(Q_OS_MACOS)
    if (InstallationInfo::currentMode() == InstallationMode::Installed) {
        return MacosUpdateInstaller::statusMessage();
    }
#endif
#if defined(Q_OS_LINUX)
    if (InstallationInfo::currentMode() == InstallationMode::Installed) {
        return LinuxUpdateInstaller::statusMessage();
    }
#endif
    return PortableUpdateInstaller::statusMessage();
}

void AppUpdateDialog::updatePlanView(const AppUpdatePlan& plan)
{
    QStringList lines;
    if (plan.updateAvailable) {
        lines << tr("Update available: %1").arg(plan.latestVersion);
        lines << QString();
        lines << tr("Selected asset:");
        lines << QStringLiteral("  %1").arg(plan.selectedAsset.fileName);
        if (plan.selectedAsset.sizeBytes > 0) {
            lines << tr("  Size: %1 bytes").arg(plan.selectedAsset.sizeBytes);
        }
        if (!plan.selectedAsset.sha256.isEmpty()) {
            lines << QStringLiteral("  SHA256: %1").arg(plan.selectedAsset.sha256);
        }
        lines << QString();
        if (!AppSettings::instance().enablePortableUpdaterPoC()
            && !AppSettings::instance().allowDevLocalAppUpdateInstall()) {
            lines << tr("Self-update installation is experimental and disabled in this RC build.");
            lines << tr("You can download and verify updates manually.");
        } else if (m_artifactVerified && m_stagingReady) {
            lines << tr("Ready to install portable update.");
            lines << tr("Zarya will close and restart.");
        } else if (m_artifactVerified) {
            lines << tr("The update artifact was downloaded and verified.");
        }
        lines << QString();
        lines << tr("Install:");
        lines << QStringLiteral("  %1").arg(installStatusMessage());
    } else {
        lines << tr("You are using the latest version for the selected channel.");
    }

    for (const QString& warning : plan.warnings) {
        lines << QString();
        lines << tr("Warning: %1").arg(warning);
    }
    for (const QString& blocker : plan.blockers) {
        lines << QString();
        lines << tr("Blocked: %1").arg(blocker);
    }

    m_detailsText->setPlainText(lines.join(QStringLiteral("\n")));
    m_downloadButton->setEnabled(plan.updateAvailable);
    refreshInstallButtonState();
}

void AppUpdateDialog::onCheckNow()
{
    m_artifactVerified = false;
    m_stagingReady = false;
    m_verifiedArchivePath.clear();
    m_stagingDir.clear();
    m_checker.checkForUpdates();
}

void AppUpdateDialog::onChooseLocalManifest()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose Update Manifest"), {},
        tr("Update manifest (*.json);;All files (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    m_artifactVerified = false;
    m_stagingReady = false;
    m_verifiedArchivePath.clear();
    m_stagingDir.clear();
    m_checker.setLocalManifestPath(path);
    refreshStaticInfo();
    m_checker.checkForUpdates();
}

void AppUpdateDialog::onDownloadAndVerify()
{
    if (!m_hasPlan || !m_lastPlan.updateAvailable) {
        return;
    }

    const AppSettings& settings = AppSettings::instance();
    const AppUpdateAsset& asset = m_lastPlan.selectedAsset;
    if (!AppUpdateVerifier::canDownloadAsset(asset, settings.allowUnsignedAppUpdates())) {
        QMessageBox::warning(
            this, tr("App Updates"),
            tr("This update asset has no checksum. Enable unsigned app update download in "
               "Settings or use a signed manifest asset."));
        return;
    }

    for (const QString& blocker : m_lastPlan.blockers) {
        QMessageBox::warning(this, tr("App Updates"), blocker);
        return;
    }

    AppUpdatePaths::ensureDirectories();
    const QString destination =
        QDir(AppUpdatePaths::downloadsDir()).filePath(asset.fileName);

    setBusy(true);
    setStatusText(tr("Downloading %1…").arg(asset.fileName));

    CoreDownloader downloader;
    QObject::connect(&downloader, &CoreDownloader::progress, this,
                     [this](qint64 received, qint64 total) {
                         if (total > 0) {
                             setStatusText(tr("Downloading… %1 / %2 bytes")
                                               .arg(received)
                                               .arg(total));
                         }
                     });

    const QString userAgent = QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString());
    const int timeoutMs = settings.githubApiTimeoutSeconds() * 1000;
    // downloadToFile is synchronous and emits finished before returning; do not wait on
    // QEventLoop after the call (exec() would hang forever).
    QString downloadError;
    const bool downloadOk =
        downloader.downloadToFile(QUrl(asset.url), destination, userAgent, timeoutMs,
                                  &downloadError);

    if (!downloadOk) {
        setBusy(false);
        setStatusText(tr("Download failed."));
        AppUpdateStatus::instance().recordInstallAttempt(QStringLiteral("download_failed"));
        QMessageBox::warning(this, tr("App Updates"), downloadError);
        return;
    }

    QString verifyError;
    if (!asset.sha256.isEmpty()
        && !AppUpdateVerifier::verifySha256(destination, asset.sha256, &verifyError)) {
        setBusy(false);
        setStatusText(tr("SHA256 verification failed."));
        AppSettings::instance().setLastAppUpdateVerificationStatus(QStringLiteral("sha256_failed"));
        QMessageBox::warning(this, tr("App Updates"), verifyError);
        return;
    }

    m_artifactVerified = true;
    m_verifiedArchivePath = destination;
    AppUpdateStatus::instance().recordDownloadVerified(asset.fileName);

    setStatusText(tr("Staging update…"));
    const AppUpdateStageResult staged = AppUpdateStager::stageArchive(
        destination, m_lastPlan.latestVersion, m_lastPlan.latestVersion);
    if (!staged.ok) {
        m_stagingReady = false;
        setBusy(false);
        setStatusText(tr("Staging failed."));
        QMessageBox::warning(this, tr("App Updates"), staged.error);
        updatePlanView(m_lastPlan);
        return;
    }

    m_stagingDir = staged.stagingDir;
    m_stagingReady = true;
    setBusy(false);
    setStatusText(tr("Download verified. Ready to install portable update."));
    updatePlanView(m_lastPlan);

    QString message = tr("Download completed.\nSHA256 verified.\n\n%1")
                          .arg(installStatusMessage());
    const QStringList signatureWarnings = AppUpdateVerifier::signatureWarnings(asset);
    for (const QString& warning : signatureWarnings) {
        message += QStringLiteral("\n\n") + warning;
    }
    QMessageBox::information(this, tr("App Updates"), message);
}

void AppUpdateDialog::onInstallAndRestart()
{
    if (!m_hasPlan || !m_artifactVerified || !m_stagingReady || m_controller == nullptr) {
        return;
    }

    QString reason;
    if (!PortableUpdateInstaller::canInstallPortableUpdate(
            m_lastPlan.selectedAsset, m_artifactVerified, m_stagingReady,
            m_controller->isCoreRunning(), m_isTestsRunning ? m_isTestsRunning() : false,
            killSwitchActive(), &reason)) {
        QMessageBox::warning(this, tr("App Updates"), reason);
        return;
    }

    const auto answer = QMessageBox::warning(
        this, tr("Install Update"),
        tr("Zarya will close while the updater replaces application files.\n"
           "User data under data/ will be preserved.\n\n"
           "Install and restart now?"),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    const UpdatePlan plan = PortableUpdateInstaller::buildUpdatePlan(
        m_lastPlan.currentVersion, m_lastPlan.latestVersion, m_stagingDir);
    QString launchError;
    if (!PortableUpdateInstaller::launchUpdaterAndQuit(plan, &launchError)) {
        AppUpdateStatus::instance().recordInstallAttempt(QStringLiteral("failed"));
        QMessageBox::warning(this, tr("App Updates"), launchError);
        return;
    }

    AppUpdateStatus::instance().recordInstallAttempt(QStringLiteral("started"));
    accept();
    m_controller->requestQuitForUpdate();
}

void AppUpdateDialog::onOpenDownloadsFolder()
{
    AppUpdatePaths::ensureDirectories();
    QDesktopServices::openUrl(QUrl::fromLocalFile(AppUpdatePaths::downloadsDir()));
}

void AppUpdateDialog::onCheckStarted()
{
    setBusy(true);
    setStatusText(tr("Checking for updates…"));
}

void AppUpdateDialog::onCheckFinished(const AppUpdatePlan& plan)
{
    m_lastPlan = plan;
    m_hasPlan = true;
    setBusy(false);
    if (plan.updateAvailable) {
        setStatusText(tr("Update available: %1").arg(plan.latestVersion));
    } else {
        setStatusText(tr("You are using the latest version for the selected channel."));
    }
    updatePlanView(plan);
}

void AppUpdateDialog::onCheckFailed(const QString& error)
{
    m_hasPlan = false;
    m_artifactVerified = false;
    m_stagingReady = false;
    setBusy(false);
    setStatusText(error);
    m_detailsText->clear();
    m_downloadButton->setEnabled(false);
    m_installButton->setEnabled(false);
}

} // namespace zarya
