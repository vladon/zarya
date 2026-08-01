#pragma once

#include "rulesets/RuleSetManager.h"

#include <QDialog>
#include <functional>

class QPlainTextEdit;
class QTableWidget;

namespace zarya {

class DnsManager;
class RoutingManager;
class ZaryaActionButton;
class ZaryaBodyText;

class RuleSetManagerDialog : public QDialog {
    Q_OBJECT

public:
    RuleSetManagerDialog(RuleSetManager& manager, RoutingManager& routingManager,
                         DnsManager& dnsManager,
                         const std::function<void(const QString&)>& logCallback,
                         QWidget* parent = nullptr);

private Q_SLOTS:
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
    ZaryaBodyText* m_requiredEmptyState = nullptr;
    ZaryaBodyText* m_allEmptyState = nullptr;
    ZaryaActionButton* m_importButton = nullptr;
    ZaryaActionButton* m_compileButton = nullptr;
};

} // namespace zarya
