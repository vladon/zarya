#pragma once

#include "backup/BackupManager.h"
#include "backup/BackupManifest.h"

#include <QDialog>
#include <functional>

class QTableWidget;

namespace zarya {

class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaSelector;
class ZaryaValidationMessage;

class BackupImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BackupImportDialog(BackupManager& manager, bool coreRunning, bool killSwitchActive,
                                const std::function<void(const QString&)>& logCallback,
                                QWidget* parent = nullptr,
                                const QString& initialArchivePath = {});

    bool importApplied() const { return m_importApplied; }

    ~BackupImportDialog() override;

private Q_SLOTS:
    void onBrowse();
    void onImport();

private:
    void clearPreview();
    void showPreview(const BackupManifest& manifest);
    ImportMode modeFromSelector(ZaryaSelector* selector) const;
    void cleanupStaging();

    BackupManager& m_manager;
    std::function<void(const QString&)> m_logCallback;
    bool m_coreRunning = false;
    bool m_killSwitchActive = false;
    bool m_importApplied = false;

    ZaryaBodyText* m_summaryLabel = nullptr;
    ZaryaValidationMessage* m_warningsLabel = nullptr;
    QTableWidget* m_table = nullptr;
    ZaryaCheckBox* m_machineSpecificCheck = nullptr;
    ZaryaSelector* m_profilesMode = nullptr;
    ZaryaSelector* m_subscriptionsMode = nullptr;
    ZaryaSelector* m_routingMode = nullptr;
    ZaryaSelector* m_dnsMode = nullptr;
    ZaryaSelector* m_settingsMode = nullptr;
    ZaryaActionButton* m_importButton = nullptr;

    QString m_archivePath;
    QString m_stagingDir;
    BackupManifest m_manifest;
};

} // namespace zarya
