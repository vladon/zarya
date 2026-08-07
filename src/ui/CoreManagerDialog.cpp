#include "ui/CoreManagerDialog.h"

#include "cores/CoreInstallStatus.h"
#include "cores/CorePaths.h"
#include "storage/AppSettings.h"
#include "ui/CoreManagerAccessibility.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace zarya {

CoreManagerDialog::CoreManagerDialog(CoreBinaryManager& manager,
                                       const std::function<void(const QString&)>& logCallback,
                                       QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("Core Manager"));
    resize(900, 560);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels(
        {tr("Core"), tr("Installed"), tr("Latest"), tr("Status"), tr("Path")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_detailsLabel = new ZaryaBodyText({}, this);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);

    m_checkButton = new ZaryaActionButton(tr("Check Versions"), this);
    m_updateButton = new ZaryaActionButton(tr("Update Selected"), this);
    m_updateAllButton = new ZaryaActionButton(tr("Update All"), this);
    m_rollbackButton = new ZaryaActionButton(tr("Rollback"), this);
    m_openFolderButton = new ZaryaActionButton(tr("Open Core Folder"), this);
    m_resetPathButton = new ZaryaActionButton(tr("Reset to Managed Path"), this);
    m_cancelButton = new ZaryaActionButton(tr("Cancel Download"), this);

    connect(m_checkButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onCheckVersions);
    connect(m_updateButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onUpdateSelected);
    connect(m_updateAllButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onUpdateAll);
    connect(m_rollbackButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onRollback);
    connect(m_openFolderButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onOpenFolder);
    connect(m_resetPathButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onResetManagedPath);
    connect(m_cancelButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onCancelDownload);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_checkButton);
    buttons->addWidget(m_updateButton);
    buttons->addWidget(m_updateAllButton);
    buttons->addWidget(m_rollbackButton);
    buttons->addWidget(m_openFolderButton);
    buttons->addWidget(m_resetPathButton);
    buttons->addWidget(m_cancelButton);
    buttons->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_table);
    layout->addWidget(m_detailsLabel);
    layout->addLayout(buttons);
    layout->addWidget(new ZaryaBodyText(tr("Log"), this));
    layout->addWidget(m_logView, 1);

    connect(&m_manager, &CoreBinaryManager::coresChanged, this, &CoreManagerDialog::onCoresChanged);
    connect(&m_manager, &CoreBinaryManager::logLine, this, &CoreManagerDialog::onLogLine);
    connect(&m_manager, &CoreBinaryManager::operationFinished, this,
            &CoreManagerDialog::onOperationFinished);
    connect(&m_manager, &CoreBinaryManager::downloadProgress, this,
            &CoreManagerDialog::onDownloadProgress);

    m_manager.refreshLocalState();
    refreshTable(m_manager.coreInfos());
    refreshDetails();
    configureCoreManagerAccessibility(
        m_table,
        m_logView,
        {m_checkButton,
         m_updateButton,
         m_updateAllButton,
         m_rollbackButton,
         m_openFolderButton,
         m_resetPathButton,
         m_cancelButton},
        tr("Installed cores"),
        tr("Core manager log"));
}

void CoreManagerDialog::refreshTable(const QVector<CoreInfo>& infos)
{
    m_table->setRowCount(infos.size());
    for (int row = 0; row < infos.size(); ++row) {
        const CoreInfo& info = infos.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(info.name));
        m_table->setItem(row, 1, new QTableWidgetItem(info.installedVersion));
        m_table->setItem(row, 2, new QTableWidgetItem(info.latestVersion));
        m_table->setItem(row, 3, new QTableWidgetItem(coreInstallStatusToString(info.status)));
        const QString location = info.distributionKind == CoreDistributionKind::Embedded
            ? tr("Built into Zarya")
            : info.executablePath;
        m_table->setItem(row, 4, new QTableWidgetItem(location));
    }
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) {
        m_table->selectRow(0);
    }
}

void CoreManagerDialog::refreshDetails()
{
    const CoreInfo info = m_manager.infoFor(selectedCoreType());
    const bool embedded = info.distributionKind == CoreDistributionKind::Embedded;
    QString details = embedded ? tr("Provider: Zarya App Update") + QLatin1Char('\n')
                               : tr("Provider: GitHub Releases") + QLatin1Char('\n');
    if (embedded) {
        details += tr("Distribution: Built into Zarya") + QLatin1Char('\n');
        details += tr("ABI version: %1").arg(info.abiVersion) + QLatin1Char('\n');
        details += tr("Load status: %1").arg(info.loadStatus) + QLatin1Char('\n');
        details += tr("This core is updated together with Zarya.") + QLatin1Char('\n');
    }
    if (!info.selectedAssetName.isEmpty()) {
        details += tr("Selected asset: %1").arg(info.selectedAssetName) + QLatin1Char('\n');
    }
    if (!info.checksumStatus.isEmpty()) {
        details += tr("Checksum: %1").arg(info.checksumStatus) + QLatin1Char('\n');
    }
    if (info.lastCheckedAt.isValid()) {
        details += tr("Last checked: %1").arg(info.lastCheckedAt.toString(Qt::ISODate))
                   + QLatin1Char('\n');
    }
    if (info.lastUpdatedAt.isValid()) {
        details += tr("Last updated: %1").arg(info.lastUpdatedAt.toString(Qt::ISODate))
                   + QLatin1Char('\n');
    }
    if (!info.lastError.isEmpty()) {
        details += tr("Last error: %1").arg(info.lastError) + QLatin1Char('\n');
    }
    if (!info.managed) {
        details += tr("Warning: core path is external and not managed by Zarya.")
                   + QLatin1Char('\n');
    }
    if (info.running) {
        details += tr("Warning: core is running. Stop it before updating.")
                   + QLatin1Char('\n');
    }
    m_updateButton->setEnabled(!embedded);
    m_updateAllButton->setEnabled(!embedded);
    m_rollbackButton->setEnabled(!embedded);
    m_openFolderButton->setEnabled(!embedded);
    m_resetPathButton->setEnabled(!embedded);
    m_detailsLabel->setText(details);
}

CoreType CoreManagerDialog::selectedCoreType() const
{
    const int row = m_table->currentRow();
    if (row == 1) {
        return CoreType::SingBox;
    }
    return CoreType::Xray;
}

void CoreManagerDialog::setBusy(bool busy)
{
    const bool embedded =
        m_manager.infoFor(selectedCoreType()).distributionKind == CoreDistributionKind::Embedded;
    m_checkButton->setEnabled(!busy);
    m_updateButton->setEnabled(!busy && !embedded);
    m_updateAllButton->setEnabled(!busy && !embedded);
    m_rollbackButton->setEnabled(!busy && !embedded);
    m_openFolderButton->setEnabled(!busy && !embedded);
    m_resetPathButton->setEnabled(!busy && !embedded);
    m_cancelButton->setEnabled(busy);
}

void CoreManagerDialog::onCheckVersions()
{
    setBusy(true);
    m_manager.checkLatestVersions();
}

void CoreManagerDialog::onUpdateSelected()
{
    const CoreInfo info = m_manager.infoFor(selectedCoreType());
    if (info.status == CoreInstallStatus::External) {
        UiMessagePresenter::warning(
            this, tr("Core Manager"),
            tr("This core is outside Zarya-managed directory."));
        return;
    }
    const bool allowWithoutChecksum = AppSettings::instance().allowCoreUpdateWithoutChecksum();
    setBusy(true);
    m_manager.updateCore(selectedCoreType(), allowWithoutChecksum);
}

void CoreManagerDialog::onUpdateAll()
{
    const bool allowWithoutChecksum = AppSettings::instance().allowCoreUpdateWithoutChecksum();
    setBusy(true);
    m_manager.updateAll(allowWithoutChecksum);
}

void CoreManagerDialog::onRollback()
{
    setBusy(true);
    m_manager.rollback(selectedCoreType());
}

void CoreManagerDialog::onOpenFolder()
{
    const CoreInfo info = m_manager.infoFor(selectedCoreType());
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.installDir));
}

void CoreManagerDialog::onResetManagedPath()
{
    m_manager.resetToManagedPath(selectedCoreType());
    refreshDetails();
}

void CoreManagerDialog::onCancelDownload()
{
    m_manager.cancelDownload();
    setBusy(false);
}

void CoreManagerDialog::onCoresChanged(const QVector<CoreInfo>& infos)
{
    refreshTable(infos);
    refreshDetails();
}

void CoreManagerDialog::onLogLine(const QString& line)
{
    m_logView->appendPlainText(line);
    if (m_logCallback) {
        m_logCallback(line);
    }
}

void CoreManagerDialog::onOperationFinished(bool ok, const QString& message)
{
    setBusy(false);
    if (!ok && !message.isEmpty()) {
        // Success is already visible in the log and table; avoid a blocking popup.
        UiMessagePresenter::warning(this, tr("Core Manager"), message);
    }
    m_manager.refreshLocalState();
}

void CoreManagerDialog::onDownloadProgress(CoreType type, qint64 received, qint64 total)
{
    const QString coreName = type == CoreType::Xray ? QStringLiteral("Xray") : QStringLiteral("sing-box");
    if (total > 0) {
        m_detailsLabel->setText(tr("Downloading %1: %2 / %3 bytes")
                                    .arg(coreName)
                                    .arg(received)
                                    .arg(total));
    }
}

} // namespace zarya
