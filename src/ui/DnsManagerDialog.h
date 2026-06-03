#pragma once

#include "dns/DnsManager.h"

#include <QDialog>
#include <functional>

class QTableWidget;

namespace zarya {

class DnsManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit DnsManagerDialog(DnsManager& manager,
                              const std::function<void(const QString&)>& logCallback,
                              QWidget* parent = nullptr);

signals:
    void activeProfileChanged(const QString& profileName);

private slots:
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
};

} // namespace zarya
