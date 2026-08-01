#include "ui/DnsServerEditorDialog.h"

#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QVBoxLayout>

namespace zarya {

DnsServerEditorDialog::DnsServerEditorDialog(const DnsServer& server, QWidget* parent)
    : QDialog(parent)
    , m_server(server)
{
    setWindowTitle(tr("DNS Server"));
    resize(520, 420);

    const auto key = [](int value) { return QString::number(value); };

    m_addressEdit = new ZaryaTextField(tr("Address"), this);
    m_addressEdit->setText(server.address);
    m_kindCombo = new ZaryaSelector(this);
    m_kindCombo->setItems({
        {key(static_cast<int>(DnsServerKind::PlainIp)),
         dnsServerKindDisplayString(DnsServerKind::PlainIp)},
        {key(static_cast<int>(DnsServerKind::DoH)),
         dnsServerKindDisplayString(DnsServerKind::DoH)},
        {key(static_cast<int>(DnsServerKind::Local)),
         dnsServerKindDisplayString(DnsServerKind::Local)},
    }, key(static_cast<int>(server.kind)));

    m_portSpin = new ZaryaNumberField(tr("Port"), 0, 65535, this);
    m_portSpin->setValue(server.port);

    m_domainsEdit = new ZaryaTextArea(tr("One domain per line"), this, 90);
    m_domainsEdit->setText(server.domains.join(QStringLiteral("\n")));
    m_expectIpsEdit = new ZaryaTextArea(tr("One expected IP per line"), this, 90);
    m_expectIpsEdit->setText(server.expectIPs.join(QStringLiteral("\n")));
    m_tagEdit = new ZaryaTextField(tr("Tag"), this);
    m_tagEdit->setText(server.tag);
    m_timeoutSpin = new ZaryaNumberField(tr("Timeout (ms)"), 0, 600000, this);
    m_timeoutSpin->setValue(server.timeoutMs);
    m_skipFallbackCheck = new ZaryaCheckBox(
        tr("Skip fallback"), this, server.skipFallback);
    m_noteEdit = new ZaryaTextField(tr("Note"), this);
    m_noteEdit->setText(server.note);

    auto* actions = new ZaryaDialogActionRow(tr("OK"), tr("Cancel"), this);
    connect(actions, &ZaryaDialogActionRow::accepted, this, &QDialog::accept);
    connect(actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(new ZaryaFormRow(tr("Address"), m_addressEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Kind"), m_kindCombo, this));
    layout->addWidget(new ZaryaFormRow(tr("Port"), m_portSpin, this));
    layout->addWidget(new ZaryaFormRow(tr("Domains"), m_domainsEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Expect IPs"), m_expectIpsEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Tag"), m_tagEdit, this));
    layout->addWidget(new ZaryaFormRow(tr("Timeout (ms)"), m_timeoutSpin, this));
    layout->addWidget(m_skipFallbackCheck);
    layout->addWidget(new ZaryaFormRow(tr("Note"), m_noteEdit, this));
    layout->addWidget(actions);
    resize(620, 650);
}

DnsServer DnsServerEditorDialog::server() const
{
    DnsServer server = m_server;
    server.address = m_addressEdit->text().trimmed();
    server.kind = static_cast<DnsServerKind>(m_kindCombo->currentKey().toInt());
    server.port = m_portSpin->value();
    server.domains = m_domainsEdit->text().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    server.expectIPs = m_expectIpsEdit->text().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    server.tag = m_tagEdit->text().trimmed();
    server.timeoutMs = m_timeoutSpin->value();
    server.skipFallback = m_skipFallbackCheck->isChecked();
    server.note = m_noteEdit->text().trimmed();
    return server;
}

} // namespace zarya
