#include "ui/BackupExportDialog.h"

#include "backup/BackupCategory.h"
#include "storage/AppPaths.h"

#include <QCheckBox>
#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace zarya {

BackupExportDialog::BackupExportDialog(BackupManager& manager,
                                       const std::function<void(const QString&)>& logCallback,
                                       QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(QStringLiteral("Export Backup"));
    resize(520, 480);

    m_fullBackupRadio = new QRadioButton(QStringLiteral("Full configuration backup"), this);
    m_diagnosticBackupRadio =
        new QRadioButton(QStringLiteral("Redacted diagnostic backup"), this);
    m_fullBackupRadio->setChecked(true);

    auto* typeGroup = new QGroupBox(QStringLiteral("Backup type"), this);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    typeLayout->addWidget(m_fullBackupRadio);
    typeLayout->addWidget(m_diagnosticBackupRadio);

    auto* includeGroup = new QGroupBox(QStringLiteral("Include"), this);
    auto* includeLayout = new QVBoxLayout(includeGroup);
    m_profilesCheck = new QCheckBox(backupCategoryDisplayName(BackupCategory::Profiles), this);
    m_subscriptionsCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::Subscriptions), this);
    m_routingCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::RoutingProfiles), this);
    m_dnsCheck = new QCheckBox(backupCategoryDisplayName(BackupCategory::DnsProfiles), this);
    m_settingsCheck = new QCheckBox(backupCategoryDisplayName(BackupCategory::AppSettings), this);
    m_geoSettingsCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::GeoDataSettings), this);
    m_ruleSetMetaCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::SingBoxRuleSetMetadata), this);
    m_ruleSetFilesCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::SingBoxRuleSetFiles), this);
    m_geoFilesCheck =
        new QCheckBox(backupCategoryDisplayName(BackupCategory::XrayGeoDataFiles), this);
    m_coreMetaCheck = new QCheckBox(backupCategoryDisplayName(BackupCategory::CoreMetadata), this);

    for (QCheckBox* check :
         {m_profilesCheck, m_subscriptionsCheck, m_routingCheck, m_dnsCheck, m_settingsCheck,
          m_geoSettingsCheck, m_ruleSetMetaCheck, m_coreMetaCheck}) {
        check->setChecked(true);
        includeLayout->addWidget(check);
    }
    includeLayout->addWidget(m_ruleSetFilesCheck);
    includeLayout->addWidget(m_geoFilesCheck);

    const QString defaultName =
        QStringLiteral("zarya-backup-%1.zarya-backup.zip")
            .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    m_outputEdit = new QLineEdit(defaultName, this);
    auto* browseButton = new QPushButton(QStringLiteral("Browse…"), this);
    connect(browseButton, &QPushButton::clicked, this, &BackupExportDialog::onBrowse);

    auto* outputRow = new QHBoxLayout;
    outputRow->addWidget(m_outputEdit, 1);
    outputRow->addWidget(browseButton);

    m_exportButton = new QPushButton(QStringLiteral("Export"), this);
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    connect(m_exportButton, &QPushButton::clicked, this, &BackupExportDialog::onExport);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_exportButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(typeGroup);
    layout->addWidget(includeGroup);
    layout->addWidget(new QLabel(QStringLiteral("Output"), this));
    layout->addLayout(outputRow);
    layout->addLayout(buttons);

    connect(m_diagnosticBackupRadio, &QRadioButton::toggled, this,
            [this](bool checked) {
                if (checked) {
                    for (QCheckBox* check :
                         {m_profilesCheck, m_subscriptionsCheck, m_routingCheck, m_dnsCheck,
                          m_settingsCheck, m_geoSettingsCheck, m_ruleSetMetaCheck}) {
                        check->setChecked(true);
                    }
                    m_ruleSetFilesCheck->setChecked(false);
                    m_geoFilesCheck->setChecked(false);
                    m_coreMetaCheck->setChecked(true);
                }
            });
}

void BackupExportDialog::onBrowse()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Backup"), m_outputEdit->text(),
        QStringLiteral("Zarya Backup (*.zarya-backup.zip)"));
    if (!path.isEmpty()) {
        m_outputEdit->setText(path);
    }
}

qint64 BackupExportDialog::estimateSelectedSizeBytes() const
{
    qint64 total = 0;
    const auto addFile = [&](const QString& path) {
        if (QFile::exists(path)) {
            total += QFileInfo(path).size();
        }
    };
    if (m_geoFilesCheck->isChecked()) {
        const QString dir = AppPaths::xrayResourceDir();
        addFile(QDir(dir).filePath(QStringLiteral("geoip.dat")));
        addFile(QDir(dir).filePath(QStringLiteral("geosite.dat")));
    }
    if (m_ruleSetFilesCheck->isChecked()) {
        for (const QString& file : QDir(AppPaths::singBoxRuleSetDir()).entryList(QDir::Files)) {
            addFile(QDir(AppPaths::singBoxRuleSetDir()).filePath(file));
        }
    }
    return total;
}

void BackupExportDialog::onExport()
{
    QString outputPath = m_outputEdit->text().trimmed();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Export Backup"),
                             QStringLiteral("Choose an output file."));
        return;
    }
    if (!outputPath.endsWith(QStringLiteral(".zarya-backup.zip"), Qt::CaseInsensitive)) {
        outputPath += QStringLiteral(".zarya-backup.zip");
    }

    const qint64 estimated = estimateSelectedSizeBytes();
    if (estimated > 20 * 1024 * 1024) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Large backup"),
            QStringLiteral("Selected optional files are about %1 MB. Continue?")
                .arg(estimated / (1024 * 1024)));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    BackupExportOptions options;
    options.outputPath = outputPath;
    options.diagnosticBackup = m_diagnosticBackupRadio->isChecked();
    options.redactionMode = options.diagnosticBackup ? BackupRedactionMode::Strict
                                                     : BackupRedactionMode::None;

    if (m_profilesCheck->isChecked()) {
        options.categories.insert(BackupCategory::Profiles);
    }
    if (m_subscriptionsCheck->isChecked()) {
        options.categories.insert(BackupCategory::Subscriptions);
    }
    if (m_routingCheck->isChecked()) {
        options.categories.insert(BackupCategory::RoutingProfiles);
    }
    if (m_dnsCheck->isChecked()) {
        options.categories.insert(BackupCategory::DnsProfiles);
    }
    if (m_settingsCheck->isChecked()) {
        options.categories.insert(BackupCategory::AppSettings);
    }
    if (m_geoSettingsCheck->isChecked()) {
        options.categories.insert(BackupCategory::GeoDataSettings);
    }
    if (m_ruleSetMetaCheck->isChecked()) {
        options.categories.insert(BackupCategory::SingBoxRuleSetMetadata);
    }
    if (m_ruleSetFilesCheck->isChecked()) {
        options.categories.insert(BackupCategory::SingBoxRuleSetFiles);
    }
    if (m_geoFilesCheck->isChecked()) {
        options.categories.insert(BackupCategory::XrayGeoDataFiles);
    }
    if (m_coreMetaCheck->isChecked()) {
        options.categories.insert(BackupCategory::CoreMetadata);
    }

    QString error;
    if (!m_manager.exportBackup(options, &error)) {
        QMessageBox::critical(this, QStringLiteral("Export Backup"), error);
        return;
    }

    if (m_logCallback) {
        m_logCallback(QStringLiteral("Backup exported: %1").arg(outputPath));
    }
    QMessageBox::information(this, QStringLiteral("Export Backup"),
                             QStringLiteral("Backup created:\n%1").arg(outputPath));
    accept();
}

} // namespace zarya
