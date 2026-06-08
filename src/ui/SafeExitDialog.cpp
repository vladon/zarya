#include "ui/SafeExitDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace zarya {

SafeExitDialog::SafeExitDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Exit Zarya"));

    auto* intro = new QLabel(QStringLiteral("Zarya is running.\n\nWhat should happen?"), this);
    intro->setWordWrap(true);

    m_stopRuntimeCheck = new QCheckBox(QStringLiteral("Stop runtime"), this);
    m_stopRuntimeCheck->setChecked(true);
    m_restoreProxyCheck = new QCheckBox(QStringLiteral("Restore system proxy"), this);
    m_restoreProxyCheck->setChecked(true);
    m_disableKillSwitchCheck =
        new QCheckBox(QStringLiteral("Disable kill switch if active and configured for clean stop"),
                      this);
    m_disableKillSwitchCheck->setChecked(true);

    auto* buttons = new QDialogButtonBox(this);
    auto* exitButton = buttons->addButton(QStringLiteral("Exit Safely"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    Q_UNUSED(exitButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_stopRuntimeCheck);
    layout->addWidget(m_restoreProxyCheck);
    layout->addWidget(m_disableKillSwitchCheck);
    layout->addWidget(buttons);
    resize(420, 220);
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
