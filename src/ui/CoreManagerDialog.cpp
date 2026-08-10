#include "ui/CoreManagerDialog.h"

#include "cores/CoreInstallStatus.h"
#include "ui/CoreManagerAccessibility.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QTableWidget>
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

    m_refreshButton = new ZaryaActionButton(tr("Refresh Status"), this);
    m_closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(m_refreshButton, &ZaryaActionButton::clicked,
            this, &CoreManagerDialog::onRefreshStatus);
    connect(m_closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);
    connect(m_table, &QTableWidget::currentCellChanged,
            this, [this] { refreshDetails(); });

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_refreshButton);
    buttons->addStretch();
    buttons->addWidget(m_closeButton);

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

    m_manager.refreshLocalState();
    refreshTable(m_manager.coreInfos());
    refreshDetails();
    configureCoreManagerAccessibility(
        m_table,
        m_logView,
        {m_refreshButton, m_closeButton},
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

void CoreManagerDialog::onRefreshStatus()
{
    m_manager.refreshLocalState();
    onLogLine(tr("Embedded core status refreshed."));
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

} // namespace zarya
