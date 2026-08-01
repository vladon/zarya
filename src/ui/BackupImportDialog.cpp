#include "ui/BackupImportDialog.h"

#include "backup/BackupCategory.h"
#include "backup/BackupValidator.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace zarya {

BackupImportDialog::BackupImportDialog(BackupManager& manager, bool coreRunning,
                                       bool killSwitchActive,
                                       const std::function<void(const QString&)>& logCallback,
                                       QWidget* parent, const QString& initialArchivePath)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
    , m_coreRunning(coreRunning)
    , m_killSwitchActive(killSwitchActive)
{
    setWindowTitle(tr("Import Backup"));
    resize(720, 560);

    m_summaryLabel = new ZaryaBodyText(tr("Select a .zarya-backup.zip file."), this);
    m_warningsLabel = new ZaryaValidationMessage(this);

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels(
        {tr("Category"), tr("Items"), tr("Included")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* browseButton = new ZaryaActionButton(tr("Browse…"), this);
    connect(browseButton, &ZaryaActionButton::clicked, this, &BackupImportDialog::onBrowse);

    const auto modeKey = [](ImportMode mode) {
        return QString::number(static_cast<int>(mode));
    };
    const auto makeModeSelector = [this, modeKey](QWidget* parent, ImportMode defaultMode) {
        auto* selector = new ZaryaSelector(parent);
        selector->setItems({
            {modeKey(ImportMode::Merge), tr("Merge")},
            {modeKey(ImportMode::Replace), tr("Replace")},
            {modeKey(ImportMode::Skip), tr("Skip")},
        }, modeKey(defaultMode));
        return selector;
    };
    const auto makeSettingsModeSelector = [this, modeKey](QWidget* parent) {
        auto* selector = new ZaryaSelector(parent);
        selector->setItems({
            {modeKey(ImportMode::Skip), tr("Skip")},
            {modeKey(ImportMode::Replace), tr("Replace")},
        }, modeKey(ImportMode::Skip));
        return selector;
    };

    auto* modesSection = new ZaryaFormSection(tr("Import modes"), this);
    m_profilesMode = makeModeSelector(modesSection, ImportMode::Merge);
    m_subscriptionsMode = makeModeSelector(modesSection, ImportMode::Merge);
    m_routingMode = makeModeSelector(modesSection, ImportMode::Merge);
    m_dnsMode = makeModeSelector(modesSection, ImportMode::Merge);
    m_settingsMode = makeSettingsModeSelector(modesSection);
    modesSection->addWidget(new ZaryaFormRow(tr("Profiles"), m_profilesMode, this));
    modesSection->addWidget(
        new ZaryaFormRow(tr("Subscriptions"), m_subscriptionsMode, this));
    modesSection->addWidget(
        new ZaryaFormRow(tr("Routing profiles"), m_routingMode, this));
    modesSection->addWidget(new ZaryaFormRow(tr("DNS profiles"), m_dnsMode, this));
    modesSection->addWidget(new ZaryaFormRow(tr("Settings"), m_settingsMode, this));

    m_machineSpecificCheck =
        new ZaryaCheckBox(tr("Import machine-specific settings"), this);

    m_importButton = new ZaryaActionButton(tr("Import Selected"), this);
    m_importButton->setEnabled(false);
    auto* cancelButton = new ZaryaActionButton(tr("Cancel"), this);
    connect(m_importButton, &ZaryaActionButton::clicked, this, &BackupImportDialog::onImport);
    connect(cancelButton, &ZaryaActionButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(browseButton);
    buttons->addStretch();
    buttons->addWidget(m_importButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_warningsLabel);
    layout->addWidget(m_table, 1);
    layout->addWidget(modesSection);
    layout->addWidget(m_machineSpecificCheck);
    layout->addLayout(buttons);

    if (m_killSwitchActive || m_coreRunning) {
        m_importButton->setEnabled(false);
        m_warningsLabel->showMessage(
            BackupManager::runtimeBlockReason(m_coreRunning, m_killSwitchActive));
    }

    if (!initialArchivePath.isEmpty()) {
        m_archivePath = initialArchivePath;
        QString error;
        if (!m_manager.loadPreview(initialArchivePath, &m_manifest, &m_stagingDir, &error)) {
            UiMessagePresenter::error(this, tr("Import Backup"), error);
        } else {
            showPreview(m_manifest);
        }
    }
}

BackupImportDialog::~BackupImportDialog()
{
    cleanupStaging();
}

void BackupImportDialog::cleanupStaging()
{
    if (!m_stagingDir.isEmpty()) {
        QDir(m_stagingDir).removeRecursively();
        m_stagingDir.clear();
    }
}

ImportMode BackupImportDialog::modeFromSelector(ZaryaSelector* selector) const
{
    return static_cast<ImportMode>(selector->currentKey().toInt());
}

void BackupImportDialog::clearPreview()
{
    cleanupStaging();
    m_manifest = {};
    m_table->setRowCount(0);
    m_importButton->setEnabled(false);
    m_warningsLabel->clear();
}

void BackupImportDialog::showPreview(const BackupManifest& manifest)
{
    m_table->setRowCount(0);
    const auto addRow = [&](const QString& key, const QString& displayName) {
        const BackupCategoryEntry entry = manifest.categories.value(key);
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(displayName));
        const QString countText =
            entry.count >= 0 ? QString::number(entry.count)
                             : (entry.included ? tr("included")
                                               : tr("not included"));
        m_table->setItem(row, 1, new QTableWidgetItem(countText));
        m_table->setItem(row, 2,
                         new QTableWidgetItem(entry.included ? tr("yes")
                                                             : tr("no")));
    };

    addRow(QStringLiteral("profiles"), backupCategoryDisplayName(BackupCategory::Profiles));
    addRow(QStringLiteral("subscriptions"),
           backupCategoryDisplayName(BackupCategory::Subscriptions));
    addRow(QStringLiteral("routing"), backupCategoryDisplayName(BackupCategory::RoutingProfiles));
    addRow(QStringLiteral("dns"), backupCategoryDisplayName(BackupCategory::DnsProfiles));
    addRow(QStringLiteral("settings"), backupCategoryDisplayName(BackupCategory::AppSettings));
    addRow(QStringLiteral("ruleSets"),
           backupCategoryDisplayName(BackupCategory::SingBoxRuleSetMetadata));
    addRow(QStringLiteral("ruleSetFiles"),
           backupCategoryDisplayName(BackupCategory::SingBoxRuleSetFiles));
    addRow(QStringLiteral("geoData"), backupCategoryDisplayName(BackupCategory::XrayGeoDataFiles));

    QString summary = tr("Backup created: %1\nApp version: %2\nPlatform: %3\nPortable mode: %4\nRedacted: %5")
                          .arg(manifest.createdAt.toString(Qt::ISODate),
                               manifest.appVersion,
                               manifest.platform,
                               manifest.portableMode ? tr("yes")
                                                     : tr("no"),
                               manifest.redacted ? tr("yes")
                                                 : tr("no"));
    m_summaryLabel->setText(summary);

    QStringList warnings = manifest.warnings;
    const BackupValidationResult validation = BackupValidator::validateManifest(manifest);
    warnings.append(validation.warnings);
    if (warnings.isEmpty()) {
        m_warningsLabel->clear();
    } else {
        m_warningsLabel->showMessage(warnings.join(QStringLiteral("\n")));
    }

    if (!m_killSwitchActive && !m_coreRunning) {
        m_importButton->setEnabled(true);
    }
}

void BackupImportDialog::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import Backup"), {},
        tr("Zarya Backup (*.zarya-backup.zip)"));
    if (path.isEmpty()) {
        return;
    }

    clearPreview();
    m_archivePath = path;

    QString error;
    if (!m_manager.loadPreview(path, &m_manifest, &m_stagingDir, &error)) {
        UiMessagePresenter::error(this, tr("Import Backup"), error);
        return;
    }

    showPreview(m_manifest);
}

void BackupImportDialog::onImport()
{
    if (m_stagingDir.isEmpty()) {
        return;
    }

    if (!UiMessagePresenter::confirm(
            this, tr("Import Backup"),
            tr("A pre-import backup of the current configuration will be created before "
               "importing. Continue?"),
            tr("Continue"))) {
        return;
    }

    BackupImportOptions options;
    options.archivePath = m_archivePath;
    options.stagingDir = m_stagingDir;
    options.importMachineSpecificSettings = m_machineSpecificCheck->isChecked();
    options.categoryModes.insert(BackupCategory::Profiles, modeFromSelector(m_profilesMode));
    options.categoryModes.insert(BackupCategory::Subscriptions,
                                 modeFromSelector(m_subscriptionsMode));
    options.categoryModes.insert(
        BackupCategory::RoutingProfiles, modeFromSelector(m_routingMode));
    options.categoryModes.insert(BackupCategory::DnsProfiles, modeFromSelector(m_dnsMode));
    options.categoryModes.insert(BackupCategory::AppSettings, modeFromSelector(m_settingsMode));
    options.categoryModes.insert(BackupCategory::GeoDataSettings, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::SingBoxRuleSetMetadata, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::SingBoxRuleSetFiles, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::XrayGeoDataFiles, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::CoreMetadata, ImportMode::Skip);

    QString preImportPath;
    QString error;
    if (!m_manager.importBackup(options, m_manifest, &error, &preImportPath)) {
        UiMessagePresenter::error(this, tr("Import Backup"), error);
        return;
    }

    m_importApplied = true;
    if (m_logCallback) {
        m_logCallback(tr("Import completed. Pre-import backup: %1").arg(preImportPath));
    }
    UiMessagePresenter::information(
        this, tr("Import Backup"),
        tr("Import completed.\n\nPre-import backup:\n%1").arg(preImportPath));
    accept();
}

} // namespace zarya
