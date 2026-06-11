#include "ui/StartupRecoveryDialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

StartupRecoveryDialog::StartupRecoveryDialog(const StartupRecoveryPlan& plan, QWidget* parent)
    : QDialog(parent)
    , m_plan(plan)
{
    setWindowTitle(tr("Startup Recovery"));
    resize(520, 360);

    auto* intro = new QLabel(
        tr("Zarya did not shut down cleanly.\n\n"
           "Recommended:\n"
           "1. Run recovery actions.\n"
           "2. Create a Diagnostics Bundle.\n"
           "3. Attach it to a bug report if the problem repeats."),
        this);
    intro->setWordWrap(true);

    m_detectedLabel = new QLabel(this);
    m_detectedLabel->setWordWrap(true);
    if (plan.detectedLines.isEmpty()) {
        m_detectedLabel->setText(tr("No recovery items detected."));
    } else {
        m_detectedLabel->setText(tr("Detected:\n• ")
                                 + plan.detectedLines.join(QStringLiteral("\n• ")));
    }

    m_restoreProxyCheck =
        new QCheckBox(tr("Restore system proxy"), this);
    m_restoreProxyCheck->setChecked(plan.systemProxyMayBeEnabled);
    m_restoreProxyCheck->setEnabled(plan.systemProxyMayBeEnabled);

    m_cleanRuntimeCheck = new QCheckBox(tr("Clean runtime temp files"), this);
    m_cleanRuntimeCheck->setChecked(plan.runtimeTempFilesPresent);
    m_cleanRuntimeCheck->setEnabled(plan.runtimeTempFilesPresent);

    m_disableKillSwitchCheck = new QCheckBox(tr("Recover kill switch"), this);
    m_disableKillSwitchCheck->setChecked(plan.killSwitchMarkerPresent);
    m_disableKillSwitchCheck->setEnabled(plan.killSwitchMarkerPresent);

    m_recoverButton = new QPushButton(tr("Recover"), this);
    m_recoverButton->setDefault(true);
    auto* diagnosticsButton = new QPushButton(tr("Create Diagnostics"), this);
    auto* reportingButton = new QPushButton(tr("Open Reporting Guide"), this);
    auto* skipButton = new QPushButton(tr("Skip"), this);

    connect(m_recoverButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(diagnosticsButton, &QPushButton::clicked, this, [this]() {
        emit createDiagnosticsRequested();
    });
    connect(reportingButton, &QPushButton::clicked, this, [this]() {
        emit openReportingGuideRequested();
    });
    connect(skipButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* buttons = new QVBoxLayout;
    buttons->addWidget(m_recoverButton);
    buttons->addWidget(diagnosticsButton);
    buttons->addWidget(reportingButton);
    buttons->addWidget(skipButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_detectedLabel);
    layout->addWidget(m_restoreProxyCheck);
    layout->addWidget(m_cleanRuntimeCheck);
    layout->addWidget(m_disableKillSwitchCheck);
    layout->addLayout(buttons);
}

StartupRecoveryPlan StartupRecoveryDialog::selectedPlan() const
{
    StartupRecoveryPlan plan = m_plan;
    plan.restoreSystemProxy = m_restoreProxyCheck->isChecked();
    plan.cleanRuntimeTempFiles = m_cleanRuntimeCheck->isChecked();
    plan.disableKillSwitch = m_disableKillSwitchCheck->isChecked();
    return plan;
}

} // namespace zarya
