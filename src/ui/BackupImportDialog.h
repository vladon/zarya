#pragma once

#include "backup/BackupManager.h"
#include "backup/BackupManifest.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

namespace zarya {

class BackupImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupImportDialog(BackupManager& manager, bool coreRunning, bool killSwitchActive,
                                const std::function<void(const QString&)>& logCallback,
                                QWidget* parent = nullptr,
                                const QString& initialArchivePath = {});

    bool importApplied() const { return m_importApplied; }

    ~BackupImportDialog() override;

private slots:
    void onBrowse();
    void onImport();

private:
    void clearPreview();
    void showPreview(const BackupManifest& manifest);
    ImportMode modeFromCombo(QComboBox* combo) const;
    void cleanupStaging();

    BackupManager& m_manager;
    std::function<void(const QString&)> m_logCallback;
    bool m_coreRunning = false;
    bool m_killSwitchActive = false;
    bool m_importApplied = false;

    QLabel* m_summaryLabel = nullptr;
    QLabel* m_warningsLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QCheckBox* m_machineSpecificCheck = nullptr;
    QComboBox* m_profilesMode = nullptr;
    QComboBox* m_subscriptionsMode = nullptr;
    QComboBox* m_routingMode = nullptr;
    QComboBox* m_dnsMode = nullptr;
    QComboBox* m_settingsMode = nullptr;
    QPushButton* m_importButton = nullptr;

    QString m_archivePath;
    QString m_stagingDir;
    BackupManifest m_manifest;
};

} // namespace zarya
