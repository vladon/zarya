#include "ui/CoreManagerDialog.h"

#include "cores/CoreInstallStatus.h"
#include "cores/CorePaths.h"
#include "storage/AppSettings.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
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
    setWindowTitle(QStringLiteral("Core Manager"));
    resize(900, 560);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Core"), QStringLiteral("Installed"), QStringLiteral("Latest"),
         QStringLiteral("Status"), QStringLiteral("Path")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_detailsLabel = new QLabel(this);
    m_detailsLabel->setWordWrap(true);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);

    m_checkButton = new QPushButton(QStringLiteral("Check Versions"), this);
    m_updateButton = new QPushButton(QStringLiteral("Update Selected"), this);
    m_updateAllButton = new QPushButton(QStringLiteral("Update All"), this);
    m_rollbackButton = new QPushButton(QStringLiteral("Rollback"), this);
    m_openFolderButton = new QPushButton(QStringLiteral("Open Core Folder"), this);
    m_resetPathButton = new QPushButton(QStringLiteral("Reset to Managed Path"), this);
    m_cancelButton = new QPushButton(QStringLiteral("Cancel Download"), this);

    connect(m_checkButton, &QPushButton::clicked, this, &CoreManagerDialog::onCheckVersions);
    connect(m_updateButton, &QPushButton::clicked, this, &CoreManagerDialog::onUpdateSelected);
    connect(m_updateAllButton, &QPushButton::clicked, this, &CoreManagerDialog::onUpdateAll);
    connect(m_rollbackButton, &QPushButton::clicked, this, &CoreManagerDialog::onRollback);
    connect(m_openFolderButton, &QPushButton::clicked, this, &CoreManagerDialog::onOpenFolder);
    connect(m_resetPathButton, &QPushButton::clicked, this, &CoreManagerDialog::onResetManagedPath);
    connect(m_cancelButton, &QPushButton::clicked, this, &CoreManagerDialog::onCancelDownload);

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
    layout->addWidget(m_table);
    layout->addWidget(m_detailsLabel);
    layout->addLayout(buttons);
    layout->addWidget(new QLabel(QStringLiteral("Log"), this));
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
        m_table->setItem(row, 4, new QTableWidgetItem(info.executablePath));
    }
    if (m_table->rowCount() > 0 && m_table->currentRow() < 0) {
        m_table->selectRow(0);
    }
}

void CoreManagerDialog::refreshDetails()
{
    const CoreInfo info = m_manager.infoFor(selectedCoreType());
    QString details = QStringLiteral("Provider: GitHub Releases\n");
    if (!info.selectedAssetName.isEmpty()) {
        details += QStringLiteral("Selected asset: %1\n").arg(info.selectedAssetName);
    }
    if (!info.checksumStatus.isEmpty()) {
        details += QStringLiteral("Checksum: %1\n").arg(info.checksumStatus);
    }
    if (info.lastCheckedAt.isValid()) {
        details += QStringLiteral("Last checked: %1\n").arg(info.lastCheckedAt.toString(Qt::ISODate));
    }
    if (info.lastUpdatedAt.isValid()) {
        details += QStringLiteral("Last updated: %1\n").arg(info.lastUpdatedAt.toString(Qt::ISODate));
    }
    if (!info.lastError.isEmpty()) {
        details += QStringLiteral("Last error: %1\n").arg(info.lastError);
    }
    if (!info.managed) {
        details += QStringLiteral(
            "Warning: core path is external and not managed by Zarya.\n");
    }
    if (info.running) {
        details += QStringLiteral("Warning: core is running. Stop it before updating.\n");
    }
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
    m_checkButton->setEnabled(!busy);
    m_updateButton->setEnabled(!busy);
    m_updateAllButton->setEnabled(!busy);
    m_rollbackButton->setEnabled(!busy);
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
        QMessageBox::warning(this, QStringLiteral("Core Manager"),
                               QStringLiteral("This core is outside Zarya-managed directory."));
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
    if (!message.isEmpty()) {
        if (ok) {
            QMessageBox::information(this, QStringLiteral("Core Manager"), message);
        } else {
            QMessageBox::warning(this, QStringLiteral("Core Manager"), message);
        }
    }
    m_manager.refreshLocalState();
}

void CoreManagerDialog::onDownloadProgress(CoreType type, qint64 received, qint64 total)
{
    const QString coreName = type == CoreType::Xray ? QStringLiteral("Xray") : QStringLiteral("sing-box");
    if (total > 0) {
        m_detailsLabel->setText(QStringLiteral("Downloading %1: %2 / %3 bytes")
                                    .arg(coreName)
                                    .arg(received)
                                    .arg(total));
    }
}

} // namespace zarya
