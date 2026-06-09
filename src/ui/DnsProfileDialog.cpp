#include "ui/DnsProfileDialog.h"

#include "dns/DnsValidator.h"
#include "dns/XrayDnsGenerator.h"
#include "ui/DnsServerEditorDialog.h"
#include "ui/RoutingJsonPreviewDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace zarya {

namespace {

QStringList queryStrategyOptions()
{
    return {dnsQueryStrategyDisplayString(DnsQueryStrategy::UseSystemDefault),
            dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIP),
            dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv4),
            dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv6)};
}

DnsQueryStrategy queryStrategyFromDisplay(const QString& display)
{
    if (display == dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIP)) {
        return DnsQueryStrategy::UseIP;
    }
    if (display == dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv4)) {
        return DnsQueryStrategy::UseIPv4;
    }
    if (display == dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv6)) {
        return DnsQueryStrategy::UseIPv6;
    }
    return DnsQueryStrategy::UseSystemDefault;
}

} // namespace

DnsProfileDialog::DnsProfileDialog(const DnsProfile& profile, bool readOnly, QWidget* parent)
    : QDialog(parent)
    , m_profile(profile)
    , m_readOnly(readOnly)
{
    setWindowTitle(readOnly ? tr("View DNS Profile")
                            : tr("Edit DNS Profile"));
    resize(820, 560);

    auto* tabs = new QTabWidget(this);

    auto* generalTab = new QWidget(this);
    m_nameEdit = new QLineEdit(profile.name, generalTab);
    m_enabledCheck = new QCheckBox(tr("Enabled"), generalTab);
    m_enabledCheck->setChecked(profile.enabled);

    m_modeCombo = new QComboBox(generalTab);
    m_modeCombo->addItem(dnsProfileModeDisplayString(DnsProfileMode::System),
                         static_cast<int>(DnsProfileMode::System));
    m_modeCombo->addItem(dnsProfileModeDisplayString(DnsProfileMode::SecureRemote),
                         static_cast<int>(DnsProfileMode::SecureRemote));
    m_modeCombo->addItem(
        dnsProfileModeDisplayString(DnsProfileMode::ChinaDirectGlobalRemote),
        static_cast<int>(DnsProfileMode::ChinaDirectGlobalRemote));
    m_modeCombo->addItem(dnsProfileModeDisplayString(DnsProfileMode::Custom),
                         static_cast<int>(DnsProfileMode::Custom));
    m_modeCombo->setCurrentIndex(m_modeCombo->findData(static_cast<int>(profile.mode)));

    m_queryStrategyCombo = new QComboBox(generalTab);
    for (const QString& option : queryStrategyOptions()) {
        m_queryStrategyCombo->addItem(option);
    }
    const int strategyIndex =
        m_queryStrategyCombo->findText(dnsQueryStrategyDisplayString(profile.queryStrategy));
    m_queryStrategyCombo->setCurrentIndex(strategyIndex >= 0 ? strategyIndex : 0);

    auto* generalForm = new QFormLayout(generalTab);
    generalForm->addRow(tr("Name"), m_nameEdit);
    generalForm->addRow(tr("Mode"), m_modeCombo);
    generalForm->addRow(QString(), m_enabledCheck);
    generalForm->addRow(tr("Query strategy"), m_queryStrategyCombo);
    tabs->addTab(generalTab, tr("General"));

    auto* serversTab = new QWidget(this);
    m_serversTable = new QTableWidget(serversTab);
    m_serversTable->setColumnCount(6);
    m_serversTable->setHorizontalHeaderLabels(
        {tr("Enabled"), tr("Address"), tr("Kind"),
         tr("Domains"), tr("Expect IPs"), tr("Note")});
    m_serversTable->horizontalHeader()->setStretchLastSection(true);
    m_serversTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serversTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serversTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    refreshServersTable();

    auto* addServerButton = new QPushButton(tr("Add Server"), serversTab);
    auto* editServerButton = new QPushButton(tr("Edit Server"), serversTab);
    auto* deleteServerButton = new QPushButton(tr("Delete Server"), serversTab);
    auto* moveUpButton = new QPushButton(tr("Move Up"), serversTab);
    auto* moveDownButton = new QPushButton(tr("Move Down"), serversTab);
    connect(addServerButton, &QPushButton::clicked, this, &DnsProfileDialog::onAddServer);
    connect(editServerButton, &QPushButton::clicked, this, &DnsProfileDialog::onEditServer);
    connect(deleteServerButton, &QPushButton::clicked, this, &DnsProfileDialog::onDeleteServer);
    connect(moveUpButton, &QPushButton::clicked, this, &DnsProfileDialog::onMoveUp);
    connect(moveDownButton, &QPushButton::clicked, this, &DnsProfileDialog::onMoveDown);

    auto* serverButtons = new QHBoxLayout;
    serverButtons->addWidget(addServerButton);
    serverButtons->addWidget(editServerButton);
    serverButtons->addWidget(deleteServerButton);
    serverButtons->addWidget(moveUpButton);
    serverButtons->addWidget(moveDownButton);
    serverButtons->addStretch();

    auto* serversLayout = new QVBoxLayout(serversTab);
    serversLayout->addWidget(m_serversTable);
    serversLayout->addLayout(serverButtons);
    tabs->addTab(serversTab, tr("Servers"));

    auto* hostsTab = new QWidget(this);
    m_hostsEdit = new QPlainTextEdit(hostsTab);
    m_hostsEdit->setPlainText(hostsToText(profile.hosts));
    auto* hostsLayout = new QVBoxLayout(hostsTab);
    hostsLayout->addWidget(new QLabel(tr("One mapping per line: domain=ip or "
                                                      "domain:example.com=1.2.3.4"),
                                        hostsTab));
    hostsLayout->addWidget(m_hostsEdit);
    tabs->addTab(hostsTab, tr("Hosts"));

    auto* advancedTab = new QWidget(this);
    m_disableCacheCheck = new QCheckBox(tr("Disable DNS cache"), advancedTab);
    m_disableCacheCheck->setChecked(profile.disableCache);
    m_disableFallbackCheck = new QCheckBox(tr("Disable fallback"), advancedTab);
    m_disableFallbackCheck->setChecked(profile.disableFallback);
    m_disableFallbackIfMatchCheck =
        new QCheckBox(tr("Disable fallback if match"), advancedTab);
    m_disableFallbackIfMatchCheck->setChecked(profile.disableFallbackIfMatch);
    auto* advancedLayout = new QVBoxLayout(advancedTab);
    advancedLayout->addWidget(m_disableCacheCheck);
    advancedLayout->addWidget(m_disableFallbackCheck);
    advancedLayout->addWidget(m_disableFallbackIfMatchCheck);
    advancedLayout->addStretch();
    tabs->addTab(advancedTab, tr("Advanced"));

    auto* validateButton = new QPushButton(tr("Validate"), this);
    auto* previewButton = new QPushButton(tr("Preview DNS JSON"), this);
    connect(validateButton, &QPushButton::clicked, this, &DnsProfileDialog::onValidate);
    connect(previewButton, &QPushButton::clicked, this, &DnsProfileDialog::onPreviewDnsJson);

    auto* buttons = new QDialogButtonBox(
        readOnly ? QDialogButtonBox::Close
                 : (QDialogButtonBox::Save | QDialogButtonBox::Cancel),
        this);
    if (!readOnly) {
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    } else {
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* footer = new QHBoxLayout;
    footer->addWidget(validateButton);
    footer->addWidget(previewButton);
    footer->addStretch();
    footer->addWidget(buttons);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addLayout(footer);

    if (readOnly || profile.isBuiltIn) {
        m_nameEdit->setReadOnly(profile.isBuiltIn);
        m_modeCombo->setEnabled(!profile.isBuiltIn);
        m_enabledCheck->setEnabled(!profile.isBuiltIn);
        m_queryStrategyCombo->setEnabled(!profile.isBuiltIn);
        addServerButton->setEnabled(!profile.isBuiltIn);
        editServerButton->setEnabled(!profile.isBuiltIn);
        deleteServerButton->setEnabled(!profile.isBuiltIn);
        moveUpButton->setEnabled(!profile.isBuiltIn);
        moveDownButton->setEnabled(!profile.isBuiltIn);
        m_hostsEdit->setReadOnly(profile.isBuiltIn);
        m_disableCacheCheck->setEnabled(!profile.isBuiltIn);
        m_disableFallbackCheck->setEnabled(!profile.isBuiltIn);
        m_disableFallbackIfMatchCheck->setEnabled(!profile.isBuiltIn);
        if (profile.isBuiltIn) {
            buttons->button(QDialogButtonBox::Save)->setEnabled(false);
        }
    }
}

DnsProfile DnsProfileDialog::profile() const
{
    DnsProfile result = m_profile;
    result.name = m_nameEdit->text().trimmed();
    result.mode = static_cast<DnsProfileMode>(m_modeCombo->currentData().toInt());
    result.enabled = m_enabledCheck->isChecked();
    result.queryStrategy =
        queryStrategyFromDisplay(m_queryStrategyCombo->currentText());
    result.hosts = parseHostsText(m_hostsEdit->toPlainText());
    result.disableCache = m_disableCacheCheck->isChecked();
    result.disableFallback = m_disableFallbackCheck->isChecked();
    result.disableFallbackIfMatch = m_disableFallbackIfMatchCheck->isChecked();
    return result;
}

void DnsProfileDialog::refreshServersTable()
{
    m_serversTable->setRowCount(m_profile.servers.size());
    for (int row = 0; row < m_profile.servers.size(); ++row) {
        const DnsServer& server = m_profile.servers.at(row);
        m_serversTable->setItem(row, 0, new QTableWidgetItem(server.enabled ? tr("Yes")
                                                                            : tr("No")));
        m_serversTable->setItem(row, 1, new QTableWidgetItem(server.address));
        m_serversTable->setItem(row, 2,
                                new QTableWidgetItem(dnsServerKindDisplayString(server.kind)));
        m_serversTable->setItem(row, 3, new QTableWidgetItem(server.domains.join(QStringLiteral(", "))));
        m_serversTable->setItem(row, 4,
                                new QTableWidgetItem(server.expectIPs.join(QStringLiteral(", "))));
        m_serversTable->setItem(row, 5, new QTableWidgetItem(server.note));
    }
}

void DnsProfileDialog::onAddServer()
{
    DnsServerEditorDialog dialog(DnsServer::createDefault(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_profile.servers.append(dialog.server());
    refreshServersTable();
}

void DnsProfileDialog::onEditServer()
{
    const int row = m_serversTable->currentRow();
    if (row < 0 || row >= m_profile.servers.size()) {
        return;
    }
    DnsServerEditorDialog dialog(m_profile.servers.at(row), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_profile.servers[row] = dialog.server();
    refreshServersTable();
}

void DnsProfileDialog::onDeleteServer()
{
    const int row = m_serversTable->currentRow();
    if (row < 0 || row >= m_profile.servers.size()) {
        return;
    }
    m_profile.servers.removeAt(row);
    refreshServersTable();
}

void DnsProfileDialog::onMoveUp()
{
    const int row = m_serversTable->currentRow();
    if (row <= 0 || row >= m_profile.servers.size()) {
        return;
    }
    m_profile.servers.swapItemsAt(row, row - 1);
    refreshServersTable();
    m_serversTable->selectRow(row - 1);
}

void DnsProfileDialog::onMoveDown()
{
    const int row = m_serversTable->currentRow();
    if (row < 0 || row + 1 >= m_profile.servers.size()) {
        return;
    }
    m_profile.servers.swapItemsAt(row, row + 1);
    refreshServersTable();
    m_serversTable->selectRow(row + 1);
}

void DnsProfileDialog::onValidate()
{
    const DnsProfile current = profile();
    const QStringList warnings = DnsValidator::warnings(current);
    if (warnings.isEmpty()) {
        QMessageBox::information(this, tr("DNS validation"),
                                 tr("No validation warnings."));
        return;
    }
    QMessageBox::warning(this, tr("DNS validation"), warnings.join(QStringLiteral("\n")));
}

void DnsProfileDialog::onPreviewDnsJson()
{
    const DnsProfile current = profile();
    const XrayDnsGenerator generator;
    const QJsonObject dns = generator.generate(current);
    const QString json =
        dns.isEmpty()
            ? tr("(DNS section omitted — System DNS or no servers)")
            : QString::fromUtf8(QJsonDocument(dns).toJson(QJsonDocument::Indented));
    RoutingJsonPreviewDialog preview(json, this);
    preview.setWindowTitle(tr("DNS JSON Preview"));
    preview.exec();
}

QMap<QString, QString> DnsProfileDialog::parseHostsText(const QString& text) const
{
    QMap<QString, QString> hosts;
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (!key.isEmpty() && !value.isEmpty()) {
            hosts.insert(key, value);
        }
    }
    return hosts;
}

QString DnsProfileDialog::hostsToText(const QMap<QString, QString>& hosts) const
{
    QStringList lines;
    for (auto it = hosts.constBegin(); it != hosts.constEnd(); ++it) {
        lines.append(QStringLiteral("%1=%2").arg(it.key(), it.value()));
    }
    return lines.join(QStringLiteral("\n"));
}

} // namespace zarya
