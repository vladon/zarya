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
    setWindowTitle(QStringLiteral("Startup Recovery"));
    resize(480, 320);

    auto* intro = new QLabel(
        QStringLiteral("Zarya detected an unclean previous shutdown."), this);
    intro->setWordWrap(true);

    m_detectedLabel = new QLabel(this);
    m_detectedLabel->setWordWrap(true);
    if (plan.detectedLines.isEmpty()) {
        m_detectedLabel->setText(QStringLiteral("No recovery items detected."));
    } else {
        m_detectedLabel->setText(QStringLiteral("Detected:\n• ")
                                 + plan.detectedLines.join(QStringLiteral("\n• ")));
    }

    m_restoreProxyCheck =
        new QCheckBox(QStringLiteral("Restore system proxy"), this);
    m_restoreProxyCheck->setChecked(plan.systemProxyMayBeEnabled);
    m_restoreProxyCheck->setEnabled(plan.systemProxyMayBeEnabled);

    m_cleanRuntimeCheck = new QCheckBox(QStringLiteral("Clean runtime temp files"), this);
    m_cleanRuntimeCheck->setChecked(plan.runtimeTempFilesPresent);
    m_cleanRuntimeCheck->setEnabled(plan.runtimeTempFilesPresent);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* skipButton = buttons->addButton(QStringLiteral("Skip"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(skipButton, &QPushButton::clicked, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_detectedLabel);
    layout->addWidget(m_restoreProxyCheck);
    layout->addWidget(m_cleanRuntimeCheck);
    layout->addWidget(buttons);
}

StartupRecoveryPlan StartupRecoveryDialog::selectedPlan() const
{
    StartupRecoveryPlan plan = m_plan;
    plan.restoreSystemProxy = m_restoreProxyCheck->isChecked();
    plan.cleanRuntimeTempFiles = m_cleanRuntimeCheck->isChecked();
    return plan;
}

} // namespace zarya
