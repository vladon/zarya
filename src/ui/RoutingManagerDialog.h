#pragma once

#include "domain/RoutingProfile.h"
#include "routing/RoutingManager.h"

#include <QDialog>
#include <functional>

class QTableWidget;

namespace zarya {

class RoutingManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoutingManagerDialog(RoutingManager& manager,
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
    RoutingProfile selectedProfile() const;
    int selectedRow() const;

    RoutingManager& m_manager;
    std::function<void(const QString&)> m_logCallback;
    QTableWidget* m_table = nullptr;
};

} // namespace zarya
