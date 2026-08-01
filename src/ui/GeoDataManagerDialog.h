#pragma once

#include "geodata/GeoDataManager.h"

#include <QDialog>
#include <functional>

class QPlainTextEdit;
class QTableWidget;

namespace zarya {

class ZaryaActionButton;
class ZaryaBodyText;
class ZaryaCheckBox;
class ZaryaSelector;

class GeoDataManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit GeoDataManagerDialog(GeoDataManager& manager,
                                  const std::function<void(const QString&)>& logCallback,
                                  QWidget* parent = nullptr);

private slots:
    void onCheckStatus();
    void onUpdateGeoIp();
    void onUpdateGeoSite();
    void onUpdateAll();
    void onVerify();
    void onOpenFolder();
    void onCancelUpdate();
    void onStatusesChanged(const QVector<GeoDataFileStatus>& statuses);
    void onProgressChanged(GeoDataKind kind, qint64 received, qint64 total);
    void onUpdateFinished(bool ok);
    void onLogLine(const QString& line);
    void onSourceChanged(const QString& sourceId);
    void onOptionsChanged();

private:
    void refreshTable(const QVector<GeoDataFileStatus>& statuses);
    void setBusy(bool busy);
    QString formatBytes(qint64 bytes) const;

    GeoDataManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    ZaryaSelector* m_sourceCombo = nullptr;
    ZaryaBodyText* m_sourceDescriptionLabel = nullptr;
    ZaryaBodyText* m_targetLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    ZaryaCheckBox* m_autoCheckCheck = nullptr;
    ZaryaCheckBox* m_warnMissingCheck = nullptr;
    ZaryaActionButton* m_updateGeoIpButton = nullptr;
    ZaryaActionButton* m_updateGeoSiteButton = nullptr;
    ZaryaActionButton* m_updateAllButton = nullptr;
    ZaryaActionButton* m_cancelButton = nullptr;
};

} // namespace zarya
