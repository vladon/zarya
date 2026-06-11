#pragma once

#include "recovery/StartupRecovery.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QPushButton;

namespace zarya {

class StartupRecoveryDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartupRecoveryDialog(const StartupRecoveryPlan& plan, QWidget* parent = nullptr);

    StartupRecoveryPlan selectedPlan() const;

signals:
    void createDiagnosticsRequested();
    void openReportingGuideRequested();

private:
    StartupRecoveryPlan m_plan;
    QCheckBox* m_restoreProxyCheck = nullptr;
    QCheckBox* m_cleanRuntimeCheck = nullptr;
    QCheckBox* m_disableKillSwitchCheck = nullptr;
    QLabel* m_detectedLabel = nullptr;
    QPushButton* m_recoverButton = nullptr;
};

} // namespace zarya
