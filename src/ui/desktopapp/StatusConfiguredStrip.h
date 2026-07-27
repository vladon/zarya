#pragma once

#include "ui/widgets/StatusDashboardWidget.h"

#include <QWidget>

namespace zarya {

// Configured status strip built with desktop-app/lib_ui.
class StatusConfiguredStrip : public QWidget {
    Q_OBJECT

public:
    explicit StatusConfiguredStrip(QWidget* parent = nullptr);

    void updateModel(const StatusDashboardModel& model);

Q_SIGNALS:
    void startRequested();
    void stopRequested();
    void testRequested();
    void openLogsRequested();
    void createDiagnosticsRequested();

private:
    void relayout();
    void setRunning(bool running);

    QWidget* m_root = nullptr; // Ui::RpWidget*
    QWidget* m_layout = nullptr; // Ui::VerticalLayout*
    QWidget* m_title = nullptr; // Ui::FlatLabel*
    QWidget* m_detail = nullptr; // Ui::FlatLabel*
    QWidget* m_primary = nullptr; // Ui::RoundButton*
    QWidget* m_secondary = nullptr; // Ui::RoundButton*
    QWidget* m_diag = nullptr; // Ui::RoundButton*
    class StatusBadgeLibUiEmbed* m_badge = nullptr;
    bool m_running = false;
};

} // namespace zarya
