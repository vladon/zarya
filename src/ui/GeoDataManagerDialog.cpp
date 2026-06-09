#include "ui/GeoDataManagerDialog.h"

#include "geodata/GeoDataSource.h"
#include "storage/GeoDataSettingsStore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
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

GeoDataManagerDialog::GeoDataManagerDialog(GeoDataManager& manager,
                                           const std::function<void(const QString&)>& logCallback,
                                           QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("Geo Data Manager"));
    resize(920, 560);

    m_sourceCombo = new QComboBox(this);
    const QVector<GeoDataSource> sources = GeoDataSources::builtInSources();
    for (const GeoDataSource& source : sources) {
        m_sourceCombo->addItem(source.name, source.id);
    }
    const QString selectedId = GeoDataSettingsStore::instance().selectedSourceId();
    const int sourceIndex = m_sourceCombo->findData(selectedId);
    if (sourceIndex >= 0) {
        m_sourceCombo->setCurrentIndex(sourceIndex);
    }

    m_sourceDescriptionLabel = new QLabel(this);
    m_sourceDescriptionLabel->setWordWrap(true);

    m_targetLabel = new QLabel(this);
    m_targetLabel->setWordWrap(true);
    m_targetLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {tr("File"), tr("Status"), tr("Size"),
         tr("Modified"), QStringLiteral("SHA256")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);

    m_autoCheckCheck = new QCheckBox(tr("Check geo data status on startup"), this);
    m_autoCheckCheck->setChecked(GeoDataSettingsStore::instance().autoCheckOnStartup());

    m_warnMissingCheck =
        new QCheckBox(tr("Warn if routing uses geo rules and files are missing"), this);
    m_warnMissingCheck->setChecked(GeoDataSettingsStore::instance().warnIfMissing());

    auto* limitations = new QLabel(
        tr("Geo data files are used by Xray routing rules such as geoip:ru and geosite:ru."),
        this);
    limitations->setWordWrap(true);

    auto* checkButton = new QPushButton(tr("Check Status"), this);
    m_updateGeoIpButton = new QPushButton(tr("Update geoip.dat"), this);
    m_updateGeoSiteButton = new QPushButton(tr("Update geosite.dat"), this);
    m_updateAllButton = new QPushButton(tr("Update All"), this);
    auto* verifyButton = new QPushButton(tr("Verify"), this);
    auto* openFolderButton = new QPushButton(tr("Open Folder"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    connect(checkButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onCheckStatus);
    connect(m_updateGeoIpButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onUpdateGeoIp);
    connect(m_updateGeoSiteButton, &QPushButton::clicked, this,
            &GeoDataManagerDialog::onUpdateGeoSite);
    connect(m_updateAllButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onUpdateAll);
    connect(verifyButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onVerify);
    connect(openFolderButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onOpenFolder);
    connect(m_cancelButton, &QPushButton::clicked, this, &GeoDataManagerDialog::onCancelUpdate);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_sourceCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &GeoDataManagerDialog::onSourceChanged);
    connect(m_autoCheckCheck, &QCheckBox::toggled, this, &GeoDataManagerDialog::onOptionsChanged);
    connect(m_warnMissingCheck, &QCheckBox::toggled, this,
            &GeoDataManagerDialog::onOptionsChanged);

    connect(&m_manager, &GeoDataManager::statusChanged, this,
            &GeoDataManagerDialog::onStatusesChanged);
    connect(&m_manager, &GeoDataManager::progressChanged, this,
            &GeoDataManagerDialog::onProgressChanged);
    connect(&m_manager, &GeoDataManager::updateFinished, this,
            &GeoDataManagerDialog::onUpdateFinished);
    connect(&m_manager, &GeoDataManager::logLine, this, &GeoDataManagerDialog::onLogLine);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(checkButton);
    buttons->addWidget(m_updateGeoIpButton);
    buttons->addWidget(m_updateGeoSiteButton);
    buttons->addWidget(m_updateAllButton);
    buttons->addWidget(verifyButton);
    buttons->addStretch();
    buttons->addWidget(openFolderButton);
    buttons->addWidget(m_cancelButton);
    buttons->addWidget(closeButton);

    auto* sourceLayout = new QFormLayout;
    sourceLayout->addRow(tr("Source:"), m_sourceCombo);
    sourceLayout->addRow(QString(), m_sourceDescriptionLabel);

    auto* targetLayout = new QFormLayout;
    targetLayout->addRow(tr("Xray resource directory:"), m_targetLabel);

    auto* optionsBox = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsBox);
    optionsLayout->addWidget(m_autoCheckCheck);
    optionsLayout->addWidget(m_warnMissingCheck);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(sourceLayout);
    layout->addLayout(targetLayout);
    layout->addWidget(m_table);
    layout->addWidget(limitations);
    layout->addWidget(optionsBox);
    layout->addWidget(new QLabel(tr("Log"), this));
    layout->addWidget(m_logView, 1);
    layout->addLayout(buttons);

    onSourceChanged(m_sourceCombo->currentIndex());
    m_targetLabel->setText(m_manager.targetDirectory());
    onCheckStatus();
}

void GeoDataManagerDialog::onSourceChanged(int index)
{
    const QString sourceId = m_sourceCombo->itemData(index).toString();
    GeoDataSettingsStore::instance().setSelectedSourceId(sourceId);
    const GeoDataSource source = GeoDataSources::sourceById(sourceId);
    m_sourceDescriptionLabel->setText(source.description);
}

void GeoDataManagerDialog::onOptionsChanged()
{
    GeoDataSettingsStore::instance().setAutoCheckOnStartup(m_autoCheckCheck->isChecked());
    GeoDataSettingsStore::instance().setWarnIfMissing(m_warnMissingCheck->isChecked());
}

void GeoDataManagerDialog::onCheckStatus()
{
    m_targetLabel->setText(m_manager.targetDirectory());
    refreshTable(m_manager.checkAllStatus());
}

void GeoDataManagerDialog::onUpdateGeoIp()
{
    setBusy(true);
    m_manager.updateGeoIp();
}

void GeoDataManagerDialog::onUpdateGeoSite()
{
    setBusy(true);
    m_manager.updateGeoSite();
}

void GeoDataManagerDialog::onUpdateAll()
{
    setBusy(true);
    m_manager.updateAll();
}

void GeoDataManagerDialog::onVerify()
{
    m_manager.verifyAll();
}

void GeoDataManagerDialog::onOpenFolder()
{
    const QString directory = m_manager.targetDirectory();
    if (directory.isEmpty()) {
        QMessageBox::warning(this, tr("Geo Data Manager"),
                             tr("Xray resource directory is not configured."));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
}

void GeoDataManagerDialog::onCancelUpdate()
{
    m_manager.cancel();
}

void GeoDataManagerDialog::onStatusesChanged(const QVector<GeoDataFileStatus>& statuses)
{
    refreshTable(statuses);
}

void GeoDataManagerDialog::onProgressChanged(GeoDataKind kind, qint64 received, qint64 total)
{
    Q_UNUSED(kind);
    if (total > 0) {
        m_logView->appendPlainText(
            tr("Download progress: %1 / %2").arg(formatBytes(received), formatBytes(total)));
    }
}

void GeoDataManagerDialog::onUpdateFinished(bool ok)
{
    Q_UNUSED(ok);
    setBusy(false);
    onCheckStatus();
}

void GeoDataManagerDialog::onLogLine(const QString& line)
{
    m_logView->appendPlainText(line);
    if (m_logCallback) {
        m_logCallback(line);
    }
}

void GeoDataManagerDialog::refreshTable(const QVector<GeoDataFileStatus>& statuses)
{
    m_table->setRowCount(statuses.size());
    for (int row = 0; row < statuses.size(); ++row) {
        const GeoDataFileStatus& status = statuses.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(status.fileName));
        m_table->setItem(row, 1,
                         new QTableWidgetItem(geoDataStatusDisplayString(status.status)));
        m_table->setItem(row, 2, new QTableWidgetItem(formatBytes(status.sizeBytes)));
        m_table->setItem(
            row, 3,
            new QTableWidgetItem(status.modifiedAt.isValid()
                                     ? status.modifiedAt.toString(Qt::ISODate)
                                     : QStringLiteral("—")));
        const QString sha =
            status.sha256.isEmpty() ? QStringLiteral("—") : status.sha256.left(16) + QStringLiteral("…");
        m_table->setItem(row, 4, new QTableWidgetItem(sha));
        if (!status.error.isEmpty()) {
            m_table->item(row, 1)->setToolTip(status.error);
        }
    }
}

void GeoDataManagerDialog::setBusy(bool busy)
{
    m_updateGeoIpButton->setEnabled(!busy);
    m_updateGeoSiteButton->setEnabled(!busy);
    m_updateAllButton->setEnabled(!busy);
    m_cancelButton->setEnabled(busy);
}

QString GeoDataManagerDialog::formatBytes(qint64 bytes) const
{
    if (bytes < 1024) {
        return tr("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return tr("%1 KB").arg(bytes / 1024);
    }
    return tr("%1 MB").arg(bytes / (1024 * 1024));
}

} // namespace zarya
