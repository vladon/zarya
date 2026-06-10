#include "ui/StartupRecoveryDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

StartupRecoveryDialog::StartupRecoveryDialog(const StartupRecoveryPlan& plan, QWidget* parent)
    : QDialog(parent)
    , m_plan(plan)
{
    setWindowTitle(tr("Startup Recovery"));
    resize(480, 320);

    auto* intro = new QLabel(
        tr("Zarya detected an unclean previous shutdown."), this);
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

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* skipButton = buttons->addButton(tr("Skip"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(skipButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_detectedLabel);
    layout->addWidget(m_restoreProxyCheck);
    layout->addWidget(m_cleanRuntimeCheck);
    layout->addWidget(m_disableKillSwitchCheck);
    layout->addWidget(buttons);
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
