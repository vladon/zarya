#pragma once

#include "geodata/GeoDataManager.h"

#include <QDialog>
#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace zarya {

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
    void onSourceChanged(int index);
    void onOptionsChanged();

private:
    void refreshTable(const QVector<GeoDataFileStatus>& statuses);
    void setBusy(bool busy);
    QString formatBytes(qint64 bytes) const;

    GeoDataManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    QComboBox* m_sourceCombo = nullptr;
    QLabel* m_sourceDescriptionLabel = nullptr;
    QLabel* m_targetLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QCheckBox* m_autoCheckCheck = nullptr;
    QCheckBox* m_warnMissingCheck = nullptr;
    QPushButton* m_updateGeoIpButton = nullptr;
    QPushButton* m_updateGeoSiteButton = nullptr;
    QPushButton* m_updateAllButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace zarya
