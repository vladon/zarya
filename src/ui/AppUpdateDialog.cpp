#include "ui/AppUpdateDialog.h"

#include "app/BuildInfo.h"
#include "cores/CoreDownloader.h"
#include "packaging/InstallationMode.h"
#include "packaging/PackagingInfo.h"
#include "storage/AppSettings.h"
#include "updater/AppUpdateChannel.h"
#include "updater/AppUpdatePaths.h"
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
#include <QEventLoop>
#include <QVBoxLayout>

namespace zarya {

AppUpdateDialog::AppUpdateDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Zarya App Updates"));
    resize(640, 520);

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
    m_openDownloadsButton = new QPushButton(tr("Open Downloads Folder"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    m_downloadButton->setEnabled(false);

    connect(m_checkButton, &QPushButton::clicked, this, &AppUpdateDialog::onCheckNow);
    connect(m_chooseManifestButton, &QPushButton::clicked, this,
            &AppUpdateDialog::onChooseLocalManifest);
    connect(m_downloadButton, &QPushButton::clicked, this, &AppUpdateDialog::onDownloadAndVerify);
    connect(m_openDownloadsButton, &QPushButton::clicked, this,
            &AppUpdateDialog::onOpenDownloadsFolder);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(&m_checker, &AppUpdateChecker::updateCheckStarted, this, &AppUpdateDialog::onCheckStarted);
    connect(&m_checker, &AppUpdateChecker::updateCheckFinished, this,
            &AppUpdateDialog::onCheckFinished);
    connect(&m_checker, &AppUpdateChecker::updateCheckFailed, this, &AppUpdateDialog::onCheckFailed);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_checkButton);
    buttonRow->addWidget(m_chooseManifestButton);
    buttonRow->addWidget(m_downloadButton);
    buttonRow->addWidget(m_openDownloadsButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

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

void AppUpdateDialog::setBusy(bool busy)
{
    m_checkButton->setEnabled(!busy);
    m_chooseManifestButton->setEnabled(!busy);
    m_downloadButton->setEnabled(!busy && m_hasPlan && m_lastPlan.updateAvailable);
    m_openDownloadsButton->setEnabled(!busy);
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
#elif defined(Q_OS_LINUX)
    if (InstallationInfo::currentMode() == InstallationMode::Installed) {
        return LinuxUpdateInstaller::statusMessage();
    }
#endif
    return PortableUpdateInstaller::statusMessage();
}

void AppUpdateDialog::onCheckNow()
{
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
    QEventLoop loop;
    bool downloadOk = false;
    QString downloadError;
    QObject::connect(&downloader, &CoreDownloader::finished, &loop,
                     [&](bool success, const QString& message) {
                         downloadOk = success;
                         downloadError = message;
                         loop.quit();
                     });

    const QString userAgent = QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString());
    const int timeoutMs = settings.githubApiTimeoutSeconds() * 1000;
    downloader.downloadToFile(QUrl(asset.url), destination, userAgent, timeoutMs);
    loop.exec();

    if (!downloadOk) {
        setBusy(false);
        setStatusText(tr("Download failed."));
        QMessageBox::warning(this, tr("App Updates"), downloadError);
        return;
    }

    QString verifyError;
    if (!asset.sha256.isEmpty()
        && !AppUpdateVerifier::verifySha256(destination, asset.sha256, &verifyError)) {
        setBusy(false);
        setStatusText(tr("SHA256 verification failed."));
        QMessageBox::warning(this, tr("App Updates"), verifyError);
        return;
    }

    QStringList signatureWarnings = AppUpdateVerifier::signatureWarnings(asset);
    setBusy(false);
    setStatusText(tr("Download completed. SHA256 verified. Installation is not implemented in this "
                     "beta."));

    QString message = tr("Download completed.\nSHA256 verified.\n\n%1")
                          .arg(installStatusMessage());
    for (const QString& warning : signatureWarnings) {
        message += QStringLiteral("\n\n") + warning;
    }
    QMessageBox::information(this, tr("App Updates"), message);
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
    setBusy(false);
    setStatusText(error);
    m_detailsText->clear();
    m_downloadButton->setEnabled(false);
}

} // namespace zarya
