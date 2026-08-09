#pragma once

#include "cores/CoreBinaryManager.h"

#include <QDialog>
#include <functional>

class QPlainTextEdit;
class QTableWidget;

namespace zarya {

class ZaryaActionButton;
class ZaryaBodyText;

class CoreManagerDialog : public QDialog {
    Q_OBJECT

public:
    CoreManagerDialog(CoreBinaryManager& manager,
                      const std::function<void(const QString&)>& logCallback,
                      QWidget* parent = nullptr);

private Q_SLOTS:
    void onCheckVersions();
    void onUpdateSelected();
    void onUpdateAll();
    void onRollback();
    void onOpenFolder();
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
    ZaryaBodyText* m_detailsLabel = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    ZaryaActionButton* m_checkButton = nullptr;
    ZaryaActionButton* m_updateButton = nullptr;
    ZaryaActionButton* m_updateAllButton = nullptr;
    ZaryaActionButton* m_rollbackButton = nullptr;
    ZaryaActionButton* m_openFolderButton = nullptr;
    ZaryaActionButton* m_cancelButton = nullptr;
};

} // namespace zarya
