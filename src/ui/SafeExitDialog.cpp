#include "ui/SafeExitDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"

#include <QVBoxLayout>

namespace zarya {

SafeExitDialog::SafeExitDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Exit Zarya"));

    auto* intro = new ZaryaBodyText(tr("Zarya is running.\n\nWhat should happen?"), this);

    m_stopRuntimeCheck = new ZaryaCheckBox(tr("Stop runtime"), this, true);
    m_restoreProxyCheck = new ZaryaCheckBox(tr("Restore system proxy"), this, true);
    m_disableKillSwitchCheck =
        new ZaryaCheckBox(
            tr("Disable kill switch if active and configured for clean stop"), this, true);

    auto* actions = new ZaryaDialogActionRow(tr("Exit Safely"), tr("Cancel"), this);
    connect(actions, &ZaryaDialogActionRow::accepted, this, &QDialog::accept);
    connect(actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(intro);
    layout->addWidget(m_stopRuntimeCheck);
    layout->addWidget(m_restoreProxyCheck);
    layout->addWidget(m_disableKillSwitchCheck);
    layout->addWidget(actions);
    resize(420, 220);

    m_stopRuntimeCheck->focusProxy()->setFocus(Qt::OtherFocusReason);
}

SafeExitOptions SafeExitDialog::options() const
{
    SafeExitOptions opts;
    opts.stopRuntime = m_stopRuntimeCheck->isChecked();
    opts.restoreSystemProxy = m_restoreProxyCheck->isChecked();
    opts.disableKillSwitch = m_disableKillSwitchCheck->isChecked();
    return opts;
}

} // namespace zarya
