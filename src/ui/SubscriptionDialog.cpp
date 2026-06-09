#include "ui/SubscriptionDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace zarya {

SubscriptionDialog::SubscriptionDialog(QWidget* parent, Subscription& subscription)
    : QDialog(parent)
    , m_subscription(subscription)
{
    setWindowTitle(subscription.name.isEmpty() ? tr("Subscription")
                                               : subscription.name);

    m_nameEdit = new QLineEdit(subscription.name, this);
    m_urlEdit = new QLineEdit(subscription.url, this);
    m_enabledCheck = new QCheckBox(tr("Enabled"), this);
    m_enabledCheck->setChecked(subscription.enabled);
    m_userAgentEdit = new QLineEdit(subscription.userAgent, this);
    m_userAgentEdit->setPlaceholderText(tr("Optional; default Zarya User-Agent"));
    m_remarksEdit = new QPlainTextEdit(subscription.remarks, this);
    m_remarksEdit->setMaximumHeight(80);

    auto* form = new QFormLayout;
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(QStringLiteral("URL"), m_urlEdit);
    form->addRow(QString(), m_enabledCheck);
    form->addRow(tr("User-Agent"), m_userAgentEdit);
    form->addRow(tr("Remarks"), m_remarksEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        m_subscription.name = m_nameEdit->text().trimmed();
        m_subscription.url = m_urlEdit->text().trimmed();
        m_subscription.enabled = m_enabledCheck->isChecked();
        m_subscription.userAgent = m_userAgentEdit->text().trimmed();
        m_subscription.remarks = m_remarksEdit->toPlainText().trimmed();
        if (!m_subscription.enabled) {
            m_subscription.lastStatus = SubscriptionStatus::Disabled;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    resize(560, 280);
}

bool SubscriptionDialog::editSubscription(QWidget* parent, Subscription& subscription)
{
    SubscriptionDialog dialog(parent, subscription);
    return dialog.exec() == QDialog::Accepted;
}

} // namespace zarya
