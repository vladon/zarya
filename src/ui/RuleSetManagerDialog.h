#pragma once

#include "rulesets/RuleSetManager.h"

#include <QDialog>
#include <functional>

class QPlainTextEdit;
class QPushButton;
class QTableWidget;

namespace zarya {

class DnsManager;
class RoutingManager;

class RuleSetManagerDialog : public QDialog {
    Q_OBJECT

public:
    RuleSetManagerDialog(RuleSetManager& manager, RoutingManager& routingManager,
                         DnsManager& dnsManager,
                         const std::function<void(const QString&)>& logCallback,
                         QWidget* parent = nullptr);

private slots:
    void onCheckStatus();
    void onImportSrs();
    void onCompileJson();
    void onOpenFolder();
    void onRefreshRequired();

private:
    void refreshTables();
    QString formatBytes(qint64 bytes) const;

    RuleSetManager& m_manager;
    RoutingManager& m_routingManager;
    DnsManager& m_dnsManager;
    std::function<void(const QString&)> m_logCallback;

    QTableWidget* m_requiredTable = nullptr;
    QTableWidget* m_allTable = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QPushButton* m_importButton = nullptr;
    QPushButton* m_compileButton = nullptr;
};

} // namespace zarya
