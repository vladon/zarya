#include "ui/GeoDataManagerDialog.h"

#include "base/algorithm.h"
#include "geodata/GeoDataSource.h"
#include "storage/GeoDataSettingsStore.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
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

    m_sourceCombo = new ZaryaSelector(this);
    const QVector<GeoDataSource> sources = GeoDataSources::builtInSources();
    QVector<ZaryaSelectorItem> sourceItems;
    sourceItems.reserve(sources.size());
    for (const GeoDataSource& source : sources) {
        sourceItems.push_back({source.id, source.name});
    }
    const QString selectedId = GeoDataSettingsStore::instance().selectedSourceId();
    m_sourceCombo->setItems(std::move(sourceItems), selectedId);

    m_sourceDescriptionLabel = new ZaryaBodyText({}, this);

    m_targetLabel = new ZaryaBodyText({}, this);

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

    m_autoCheckCheck = new ZaryaCheckBox(
        tr("Check geo data status on startup"), this,
        GeoDataSettingsStore::instance().autoCheckOnStartup());

    m_warnMissingCheck =
        new ZaryaCheckBox(tr("Warn if routing uses geo rules and files are missing"), this,
                          GeoDataSettingsStore::instance().warnIfMissing());

    auto* limitations = new ZaryaBodyText(
        tr("Geo data files are used by Xray routing rules such as geoip:ru and geosite:ru."),
        this);

    auto* checkButton = new ZaryaActionButton(tr("Check Status"), this);
    m_updateGeoIpButton = new ZaryaActionButton(tr("Update geoip.dat"), this);
    m_updateGeoSiteButton = new ZaryaActionButton(tr("Update geosite.dat"), this);
    m_updateAllButton = new ZaryaActionButton(
        tr("Update All"), this, ZaryaButtonRole::Primary);
    auto* verifyButton = new ZaryaActionButton(tr("Verify"), this);
    auto* openFolderButton = new ZaryaActionButton(tr("Open Folder"), this);
    m_cancelButton = new ZaryaActionButton(tr("Cancel"), this);
    auto* closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(checkButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onCheckStatus);
    connect(m_updateGeoIpButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onUpdateGeoIp);
    connect(m_updateGeoSiteButton, &ZaryaActionButton::clicked, this,
            &GeoDataManagerDialog::onUpdateGeoSite);
    connect(m_updateAllButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onUpdateAll);
    connect(verifyButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onVerify);
    connect(openFolderButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onOpenFolder);
    connect(m_cancelButton, &ZaryaActionButton::clicked,
            this, &GeoDataManagerDialog::onCancelUpdate);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);
    connect(m_sourceCombo, &ZaryaSelector::currentKeyChanged, this,
            &GeoDataManagerDialog::onSourceChanged);
    connect(m_autoCheckCheck, &ZaryaCheckBox::toggled,
            this, &GeoDataManagerDialog::onOptionsChanged);
    connect(m_warnMissingCheck, &ZaryaCheckBox::toggled, this,
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

    auto* sourceSection = new ZaryaFormSection(tr("Source"), this);
    sourceSection->addWidget(new ZaryaFormRow(tr("Source:"), m_sourceCombo, this));
    sourceSection->addWidget(m_sourceDescriptionLabel);
    sourceSection->addWidget(
        new ZaryaFormRow(tr("Xray resource directory:"), m_targetLabel, this));

    auto* optionsSection = new ZaryaFormSection(tr("Options"), this);
    optionsSection->addWidget(m_autoCheckCheck);
    optionsSection->addWidget(m_warnMissingCheck);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(sourceSection);
    layout->addWidget(m_table);
    layout->addWidget(limitations);
    layout->addWidget(optionsSection);
    layout->addWidget(new ZaryaBodyText(tr("Log"), this));
    layout->addWidget(m_logView, 1);
    layout->addLayout(buttons);

    onSourceChanged(m_sourceCombo->currentKey());
    m_targetLabel->setText(m_manager.targetDirectory());
    onCheckStatus();
}

void GeoDataManagerDialog::onSourceChanged(const QString& sourceId)
{
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
    m_manager.checkStatus();
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
        UiMessagePresenter::warning(
            this, tr("Geo Data Manager"),
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
    m_targetLabel->setText(m_manager.targetDirectory());
    // statusChanged is already emitted by the update worker.
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
