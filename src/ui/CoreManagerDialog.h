#pragma once

#include "cores/CoreBinaryManager.h"

#include <QDialog>
#include <functional>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace zarya {

class CoreManagerDialog : public QDialog {
    Q_OBJECT

public:
    CoreManagerDialog(CoreBinaryManager& manager,
                      const std::function<void(const QString&)>& logCallback,
                      QWidget* parent = nullptr);

private slots:
    void onCheckVersions();
    void onUpdateSelected();
    void onUpdateAll();
    void onRollback();
    void onOpenFolder();
    void onResetManagedPath();
    void onCancelDownload();
    void onCoresChanged(const QVector<CoreInfo>& infos);
    void onLogLine(const QString& line);
    void onOperationFinished(bool ok, const QString& message);
    void onDownloadProgress(CoreType type, qint64 received, qint64 total);

private:
    void refreshTable(const QVector<CoreInfo>& infos);
    void refreshDetails();
    void setBusy(bool busy);
    CoreType selectedCoreType() const;

    CoreBinaryManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    QTableWidget* m_table = nullptr;
    QLabel* m_detailsLabel = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QPushButton* m_checkButton = nullptr;
    QPushButton* m_updateButton = nullptr;
    QPushButton* m_updateAllButton = nullptr;
    QPushButton* m_rollbackButton = nullptr;
    QPushButton* m_openFolderButton = nullptr;
    QPushButton* m_resetPathButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace zarya
