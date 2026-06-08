#include "ui/BackupImportDialog.h"

#include "backup/BackupCategory.h"
#include "backup/BackupValidator.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace zarya {

namespace {

QComboBox* makeModeCombo(QWidget* parent, ImportMode defaultMode)
{
    auto* combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("Merge"), static_cast<int>(ImportMode::Merge));
    combo->addItem(QStringLiteral("Replace"), static_cast<int>(ImportMode::Replace));
    combo->addItem(QStringLiteral("Skip"), static_cast<int>(ImportMode::Skip));
    const int index = combo->findData(static_cast<int>(defaultMode));
    if (index >= 0) {
        combo->setCurrentIndex(index);
    }
    return combo;
}

QComboBox* makeSettingsModeCombo(QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    combo->addItem(QStringLiteral("Skip"), static_cast<int>(ImportMode::Skip));
    combo->addItem(QStringLiteral("Replace"), static_cast<int>(ImportMode::Replace));
    combo->setCurrentIndex(0);
    return combo;
}

} // namespace

BackupImportDialog::BackupImportDialog(BackupManager& manager, bool coreRunning,
                                       bool killSwitchActive,
                                       const std::function<void(const QString&)>& logCallback,
                                       QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
    , m_coreRunning(coreRunning)
    , m_killSwitchActive(killSwitchActive)
{
    setWindowTitle(QStringLiteral("Import Backup"));
    resize(720, 560);

    m_summaryLabel = new QLabel(QStringLiteral("Select a .zarya-backup.zip file."), this);
    m_summaryLabel->setWordWrap(true);
    m_warningsLabel = new QLabel(this);
    m_warningsLabel->setWordWrap(true);
    m_warningsLabel->setStyleSheet(QStringLiteral("color: #b45309;"));

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Category"), QStringLiteral("Items"), QStringLiteral("Included")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* browseButton = new QPushButton(QStringLiteral("Browse…"), this);
    connect(browseButton, &QPushButton::clicked, this, &BackupImportDialog::onBrowse);

    auto* modesGroup = new QGroupBox(QStringLiteral("Import modes"), this);
    auto* modesForm = new QFormLayout(modesGroup);
    m_profilesMode = makeModeCombo(modesGroup, ImportMode::Merge);
    m_subscriptionsMode = makeModeCombo(modesGroup, ImportMode::Merge);
    m_routingMode = makeModeCombo(modesGroup, ImportMode::Merge);
    m_dnsMode = makeModeCombo(modesGroup, ImportMode::Merge);
    m_settingsMode = makeSettingsModeCombo(modesGroup);
    modesForm->addRow(QStringLiteral("Profiles"), m_profilesMode);
    modesForm->addRow(QStringLiteral("Subscriptions"), m_subscriptionsMode);
    modesForm->addRow(QStringLiteral("Routing profiles"), m_routingMode);
    modesForm->addRow(QStringLiteral("DNS profiles"), m_dnsMode);
    modesForm->addRow(QStringLiteral("Settings"), m_settingsMode);

    m_machineSpecificCheck =
        new QCheckBox(QStringLiteral("Import machine-specific settings"), this);
    m_machineSpecificCheck->setChecked(false);

    m_importButton = new QPushButton(QStringLiteral("Import Selected"), this);
    m_importButton->setEnabled(false);
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    connect(m_importButton, &QPushButton::clicked, this, &BackupImportDialog::onImport);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(browseButton);
    buttons->addStretch();
    buttons->addWidget(m_importButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_warningsLabel);
    layout->addWidget(m_table, 1);
    layout->addWidget(modesGroup);
    layout->addWidget(m_machineSpecificCheck);
    layout->addLayout(buttons);

    if (m_killSwitchActive || m_coreRunning) {
        m_importButton->setEnabled(false);
        m_warningsLabel->setText(BackupManager::runtimeBlockReason(m_coreRunning, m_killSwitchActive));
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

ImportMode BackupImportDialog::modeFromCombo(QComboBox* combo) const
{
    return static_cast<ImportMode>(combo->currentData().toInt());
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
                             : (entry.included ? QStringLiteral("included")
                                               : QStringLiteral("not included"));
        m_table->setItem(row, 1, new QTableWidgetItem(countText));
        m_table->setItem(row, 2,
                         new QTableWidgetItem(entry.included ? QStringLiteral("yes")
                                                             : QStringLiteral("no")));
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

    QString summary = QStringLiteral("Backup created: %1\nApp version: %2\nPlatform: %3\nPortable mode: %4\nRedacted: %5")
                          .arg(manifest.createdAt.toString(Qt::ISODate),
                               manifest.appVersion,
                               manifest.platform,
                               manifest.portableMode ? QStringLiteral("yes")
                                                     : QStringLiteral("no"),
                               manifest.redacted ? QStringLiteral("yes")
                                                 : QStringLiteral("no"));
    m_summaryLabel->setText(summary);

    QStringList warnings = manifest.warnings;
    const BackupValidationResult validation = BackupValidator::validateManifest(manifest);
    warnings.append(validation.warnings);
    m_warningsLabel->setText(warnings.isEmpty() ? QString() : warnings.join(QStringLiteral("\n")));

    if (!m_killSwitchActive && !m_coreRunning) {
        m_importButton->setEnabled(true);
    }
}

void BackupImportDialog::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Import Backup"), {},
        QStringLiteral("Zarya Backup (*.zarya-backup.zip)"));
    if (path.isEmpty()) {
        return;
    }

    clearPreview();
    m_archivePath = path;

    QString error;
    if (!m_manager.loadPreview(path, &m_manifest, &m_stagingDir, &error)) {
        QMessageBox::critical(this, QStringLiteral("Import Backup"), error);
        return;
    }

    showPreview(m_manifest);
}

void BackupImportDialog::onImport()
{
    if (m_stagingDir.isEmpty()) {
        return;
    }

    const auto confirm = QMessageBox::question(
        this, QStringLiteral("Import Backup"),
        QStringLiteral("A pre-import backup of the current configuration will be created before "
                       "importing. Continue?"));
    if (confirm != QMessageBox::Yes) {
        return;
    }

    BackupImportOptions options;
    options.archivePath = m_archivePath;
    options.stagingDir = m_stagingDir;
    options.importMachineSpecificSettings = m_machineSpecificCheck->isChecked();
    options.categoryModes.insert(BackupCategory::Profiles, modeFromCombo(m_profilesMode));
    options.categoryModes.insert(BackupCategory::Subscriptions,
                                 modeFromCombo(m_subscriptionsMode));
    options.categoryModes.insert(BackupCategory::RoutingProfiles, modeFromCombo(m_routingMode));
    options.categoryModes.insert(BackupCategory::DnsProfiles, modeFromCombo(m_dnsMode));
    options.categoryModes.insert(BackupCategory::AppSettings, modeFromCombo(m_settingsMode));
    options.categoryModes.insert(BackupCategory::GeoDataSettings, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::SingBoxRuleSetMetadata, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::SingBoxRuleSetFiles, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::XrayGeoDataFiles, ImportMode::Merge);
    options.categoryModes.insert(BackupCategory::CoreMetadata, ImportMode::Skip);

    QString preImportPath;
    QString error;
    if (!m_manager.importBackup(options, m_manifest, &error, &preImportPath)) {
        QMessageBox::critical(this, QStringLiteral("Import Backup"), error);
        return;
    }

    m_importApplied = true;
    if (m_logCallback) {
        m_logCallback(QStringLiteral("Import completed. Pre-import backup: %1").arg(preImportPath));
    }
    QMessageBox::information(
        this, QStringLiteral("Import Backup"),
        QStringLiteral("Import completed.\n\nPre-import backup:\n%1").arg(preImportPath));
    accept();
}

} // namespace zarya
