#include "ui/DnsServerEditorDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace zarya {

DnsServerEditorDialog::DnsServerEditorDialog(const DnsServer& server, QWidget* parent)
    : QDialog(parent)
    , m_server(server)
{
    setWindowTitle(tr("DNS Server"));
    resize(520, 420);

    m_addressEdit = new QLineEdit(server.address, this);
    m_kindCombo = new QComboBox(this);
    m_kindCombo->addItem(dnsServerKindDisplayString(DnsServerKind::PlainIp),
                         static_cast<int>(DnsServerKind::PlainIp));
    m_kindCombo->addItem(dnsServerKindDisplayString(DnsServerKind::DoH),
                         static_cast<int>(DnsServerKind::DoH));
    m_kindCombo->addItem(dnsServerKindDisplayString(DnsServerKind::Local),
                         static_cast<int>(DnsServerKind::Local));
    m_kindCombo->setCurrentIndex(m_kindCombo->findData(static_cast<int>(server.kind)));

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(0, 65535);
    m_portSpin->setValue(server.port);

    m_domainsEdit = new QPlainTextEdit(this);
    m_domainsEdit->setPlainText(server.domains.join(QStringLiteral("\n")));
    m_expectIpsEdit = new QPlainTextEdit(this);
    m_expectIpsEdit->setPlainText(server.expectIPs.join(QStringLiteral("\n")));
    m_tagEdit = new QLineEdit(server.tag, this);
    m_timeoutSpin = new QSpinBox(this);
    m_timeoutSpin->setRange(0, 600000);
    m_timeoutSpin->setValue(server.timeoutMs);
    m_skipFallbackCheck = new QCheckBox(tr("Skip fallback"), this);
    m_skipFallbackCheck->setChecked(server.skipFallback);
    m_noteEdit = new QLineEdit(server.note, this);

    auto* form = new QFormLayout;
    form->addRow(tr("Address"), m_addressEdit);
    form->addRow(tr("Kind"), m_kindCombo);
    form->addRow(tr("Port"), m_portSpin);
    form->addRow(tr("Domains"), m_domainsEdit);
    form->addRow(tr("Expect IPs"), m_expectIpsEdit);
    form->addRow(tr("Tag"), m_tagEdit);
    form->addRow(tr("Timeout (ms)"), m_timeoutSpin);
    form->addRow(QString(), m_skipFallbackCheck);
    form->addRow(tr("Note"), m_noteEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

DnsServer DnsServerEditorDialog::server() const
{
    DnsServer server = m_server;
    server.address = m_addressEdit->text().trimmed();
    server.kind = static_cast<DnsServerKind>(m_kindCombo->currentData().toInt());
    server.port = m_portSpin->value();
    server.domains = m_domainsEdit->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    server.expectIPs = m_expectIpsEdit->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    server.tag = m_tagEdit->text().trimmed();
    server.timeoutMs = m_timeoutSpin->value();
    server.skipFallback = m_skipFallbackCheck->isChecked();
    server.note = m_noteEdit->text().trimmed();
    return server;
}

} // namespace zarya
