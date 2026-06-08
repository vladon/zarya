#pragma once

#include "backup/BackupManager.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;

namespace zarya {

class BackupExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupExportDialog(BackupManager& manager,
                                const std::function<void(const QString&)>& logCallback,
                                QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onExport();

private:
    qint64 estimateSelectedSizeBytes() const;

    BackupManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    QRadioButton* m_fullBackupRadio = nullptr;
    QRadioButton* m_diagnosticBackupRadio = nullptr;
    QCheckBox* m_profilesCheck = nullptr;
    QCheckBox* m_subscriptionsCheck = nullptr;
    QCheckBox* m_routingCheck = nullptr;
    QCheckBox* m_dnsCheck = nullptr;
    QCheckBox* m_settingsCheck = nullptr;
    QCheckBox* m_geoSettingsCheck = nullptr;
    QCheckBox* m_ruleSetMetaCheck = nullptr;
    QCheckBox* m_ruleSetFilesCheck = nullptr;
    QCheckBox* m_geoFilesCheck = nullptr;
    QCheckBox* m_coreMetaCheck = nullptr;
    QLineEdit* m_outputEdit = nullptr;
    QPushButton* m_exportButton = nullptr;
};

} // namespace zarya
