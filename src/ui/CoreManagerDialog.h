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
    void onRefreshStatus();
    void onCoresChanged(const QVector<CoreInfo>& infos);
    void onLogLine(const QString& line);

private:
    void refreshTable(const QVector<CoreInfo>& infos);
    void refreshDetails();
    CoreType selectedCoreType() const;

    CoreBinaryManager& m_manager;
    std::function<void(const QString&)> m_logCallback;

    QTableWidget* m_table = nullptr;
    ZaryaBodyText* m_detailsLabel = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    ZaryaActionButton* m_refreshButton = nullptr;
    ZaryaActionButton* m_closeButton = nullptr;
};

} // namespace zarya
