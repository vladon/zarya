#include "ui/SubscriptionDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"

#include <QVBoxLayout>

namespace zarya {

SubscriptionDialog::SubscriptionDialog(QWidget* parent, Subscription& subscription)
    : QDialog(parent)
    , m_subscription(subscription)
{
    setWindowTitle(subscription.name.isEmpty() ? tr("Subscription")
                                               : subscription.name);

    m_nameEdit = new ZaryaTextField(tr("Name"), this);
    m_nameEdit->setText(subscription.name);
    m_urlEdit = new ZaryaTextField(QStringLiteral("URL"), this);
    m_urlEdit->setText(subscription.url);
    m_enabledCheck = new ZaryaCheckBox(tr("Enabled"), this, subscription.enabled);
    m_userAgentEdit = new ZaryaTextField(
        tr("Optional; default Zarya User-Agent"), this);
    m_userAgentEdit->setText(subscription.userAgent);
    m_remarksEdit = new ZaryaTextArea(tr("Remarks"), this, 80);
    m_remarksEdit->setText(subscription.remarks);

    auto* actions = new ZaryaDialogActionRow(tr("OK"), tr("Cancel"), this);
    connect(actions, &ZaryaDialogActionRow::accepted, this, [this]() {
        m_subscription.name = m_nameEdit->text().trimmed();
        m_subscription.url = m_urlEdit->text().trimmed();
        m_subscription.enabled = m_enabledCheck->isChecked();
        m_subscription.userAgent = m_userAgentEdit->text().trimmed();
        m_subscription.remarks = m_remarksEdit->text().trimmed();
        if (!m_subscription.enabled) {
            m_subscription.lastStatus = SubscriptionStatus::Disabled;
        }
        accept();
    });
    connect(actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(new ZaryaFormRow(tr("Name"), m_nameEdit, this));
    layout->addWidget(new ZaryaFormRow(QStringLiteral("URL"), m_urlEdit, this));
    layout->addWidget(m_enabledCheck);
    layout->addWidget(new ZaryaFormRow(tr("User-Agent"), m_userAgentEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Remarks"), m_remarksEdit, this));
    layout->addWidget(actions);
    resize(600, 420);
    m_nameEdit->setFocus(Qt::OtherFocusReason);
}

bool SubscriptionDialog::editSubscription(QWidget* parent, Subscription& subscription)
{
    SubscriptionDialog dialog(parent, subscription);
    return dialog.exec() == QDialog::Accepted;
}

} // namespace zarya
