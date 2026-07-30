#pragma once

#include "ui/widgets/StatusDashboardWidget.h"

#include <QWidget>

namespace zarya {

class StatusUnconfiguredPanel final : public QWidget {
    Q_OBJECT

public:
    explicit StatusUnconfiguredPanel(QWidget* parent = nullptr);

    void updateModel(const StatusDashboardModel& model);

Q_SIGNALS:
    void openCoreManagerRequested();
    void addProfileRequested();
    void addSubscriptionRequested();
    void runSetupRequested();
    void pasteLinkRequested();
    void importBackupRequested();

private:
    void relayout();

    QWidget* m_root = nullptr; // Ui::RpWidget*
    QWidget* m_layout = nullptr; // Ui::VerticalLayout*
    QWidget* m_steps = nullptr; // Ui::FlatLabel*
    QWidget* m_coreButton = nullptr; // Ui::RoundButton*
};

} // namespace zarya
