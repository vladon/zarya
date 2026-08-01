#pragma once

#include "dns/DnsManager.h"

#include <QDialog>
#include <functional>

class QTableWidget;

namespace zarya {

class ZaryaBodyText;

class DnsManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsManagerDialog(DnsManager& manager,
                              const std::function<void(const QString&)>& logCallback,
                              QWidget* parent = nullptr);

Q_SIGNALS:
    void activeProfileChanged(const QString& profileName);

private Q_SLOTS:
    void onNew();
    void onEdit();
    void onDuplicate();
    void onDelete();
    void onSetActive();
    void onPreview();
    void refreshTable();

private:
    DnsProfile selectedProfile() const;
    int selectedRow() const;
    QString flagsText(const DnsProfile& profile) const;

    DnsManager& m_manager;
    std::function<void(const QString&)> m_logCallback;
    QTableWidget* m_table = nullptr;
    ZaryaBodyText* m_emptyState = nullptr;
};

} // namespace zarya
