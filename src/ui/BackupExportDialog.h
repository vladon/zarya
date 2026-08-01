#pragma once

#include "backup/BackupManager.h"

#include <QDialog>
#include <functional>

namespace zarya {

class ZaryaCheckBox;
class ZaryaRadioGroup;
class ZaryaTextField;

class BackupExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupExportDialog(BackupManager& manager,
                                const std::function<void(const QString&)>& logCallback,
                                QWidget* parent = nullptr);

private Q_SLOTS:
    void onBrowse();
    void onExport();

private:
    qint64 estimateSelectedSizeBytes() const;

    BackupManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    ZaryaRadioGroup* m_backupType = nullptr;
    ZaryaCheckBox* m_profilesCheck = nullptr;
    ZaryaCheckBox* m_subscriptionsCheck = nullptr;
    ZaryaCheckBox* m_routingCheck = nullptr;
    ZaryaCheckBox* m_dnsCheck = nullptr;
    ZaryaCheckBox* m_settingsCheck = nullptr;
    ZaryaCheckBox* m_geoSettingsCheck = nullptr;
    ZaryaCheckBox* m_ruleSetMetaCheck = nullptr;
    ZaryaCheckBox* m_ruleSetFilesCheck = nullptr;
    ZaryaCheckBox* m_geoFilesCheck = nullptr;
    ZaryaCheckBox* m_coreMetaCheck = nullptr;
    ZaryaTextField* m_outputEdit = nullptr;
};

} // namespace zarya
