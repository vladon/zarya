#include "ui/BackupExportDialog.h"

#include "backup/BackupCategory.h"
#include "storage/AppPaths.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QDate>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace zarya {

BackupExportDialog::BackupExportDialog(BackupManager& manager,
                                       const std::function<void(const QString&)>& logCallback,
                                       QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("Export Backup"));
    resize(520, 480);

    m_backupType = new ZaryaRadioGroup(0, this);
    m_backupType->addOption(0, tr("Full configuration backup"));
    m_backupType->addOption(1, tr("Redacted diagnostic backup"));
    m_backupType->setAccessibleLabel(tr("Backup type"));
    auto* typeSection = new ZaryaFormSection(tr("Backup type"), this);
    typeSection->addWidget(m_backupType);

    auto* includeSection = new ZaryaFormSection(tr("Include"), this);
    m_profilesCheck = new ZaryaCheckBox(
        backupCategoryDisplayName(BackupCategory::Profiles), this);
    m_subscriptionsCheck =
        new ZaryaCheckBox(backupCategoryDisplayName(BackupCategory::Subscriptions), this);
    m_routingCheck =
        new ZaryaCheckBox(backupCategoryDisplayName(BackupCategory::RoutingProfiles), this);
    m_dnsCheck = new ZaryaCheckBox(
        backupCategoryDisplayName(BackupCategory::DnsProfiles), this);
    m_settingsCheck = new ZaryaCheckBox(
        backupCategoryDisplayName(BackupCategory::AppSettings), this);
    m_geoSettingsCheck =
        new ZaryaCheckBox(backupCategoryDisplayName(BackupCategory::GeoDataSettings), this);
    m_ruleSetMetaCheck =
        new ZaryaCheckBox(
            backupCategoryDisplayName(BackupCategory::SingBoxRuleSetMetadata), this);
    m_ruleSetFilesCheck =
        new ZaryaCheckBox(
            backupCategoryDisplayName(BackupCategory::SingBoxRuleSetFiles), this);
    m_geoFilesCheck =
        new ZaryaCheckBox(backupCategoryDisplayName(BackupCategory::XrayGeoDataFiles), this);
    m_coreMetaCheck = new ZaryaCheckBox(
        backupCategoryDisplayName(BackupCategory::CoreMetadata), this);

    for (ZaryaCheckBox* check :
         {m_profilesCheck, m_subscriptionsCheck, m_routingCheck, m_dnsCheck, m_settingsCheck,
          m_geoSettingsCheck, m_ruleSetMetaCheck, m_coreMetaCheck}) {
        check->setChecked(true);
        includeSection->addWidget(check);
    }
    includeSection->addWidget(m_ruleSetFilesCheck);
    includeSection->addWidget(m_geoFilesCheck);

    const QString defaultName =
        QStringLiteral("zarya-backup-%1.zarya-backup.zip")
            .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    m_outputEdit = new ZaryaTextField(tr("Output"), this);
    m_outputEdit->setAccessibleLabel(tr("Output"));
    m_outputEdit->setText(defaultName);
    auto* browseButton = new ZaryaActionButton(tr("Browse…"), this);
    connect(browseButton, &ZaryaActionButton::clicked, this, &BackupExportDialog::onBrowse);

    auto* outputRow = new QHBoxLayout;
    outputRow->addWidget(m_outputEdit, 1);
    outputRow->addWidget(browseButton);

    auto* actions = new ZaryaDialogActionRow(tr("Export"), tr("Cancel"), this);
    connect(actions, &ZaryaDialogActionRow::accepted, this, &BackupExportDialog::onExport);
    connect(actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(typeSection);
    layout->addWidget(includeSection);
    layout->addLayout(outputRow);
    layout->addWidget(actions);

    m_backupType->focusProxy()->setFocus(Qt::OtherFocusReason);

    connect(m_backupType, &ZaryaRadioGroup::valueChanged, this,
            [this](int value) {
                if (value == 1) {
                    for (ZaryaCheckBox* check :
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
        this, tr("Export Backup"), m_outputEdit->text(),
        tr("Zarya Backup (*.zarya-backup.zip)"));
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
        UiMessagePresenter::warning(
            this, tr("Export Backup"), tr("Choose an output file."));
        return;
    }
    if (!outputPath.endsWith(QStringLiteral(".zarya-backup.zip"), Qt::CaseInsensitive)) {
        outputPath += QStringLiteral(".zarya-backup.zip");
    }

    const qint64 estimated = estimateSelectedSizeBytes();
    if (estimated > 20 * 1024 * 1024) {
        if (!UiMessagePresenter::confirm(
                this, tr("Large backup"),
                tr("Selected optional files are about %1 MB. Continue?")
                    .arg(estimated / (1024 * 1024)),
                tr("Continue"))) {
            return;
        }
    }

    BackupExportOptions options;
    options.outputPath = outputPath;
    options.diagnosticBackup = (m_backupType->value() == 1);
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
        UiMessagePresenter::error(this, tr("Export Backup"), error);
        return;
    }

    if (m_logCallback) {
        m_logCallback(tr("Backup exported: %1").arg(outputPath));
    }
    UiMessagePresenter::information(
        this, tr("Export Backup"), tr("Backup created:\n%1").arg(outputPath));
    accept();
}

} // namespace zarya
