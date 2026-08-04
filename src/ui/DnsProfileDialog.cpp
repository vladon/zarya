#include "ui/DnsProfileDialog.h"

#include "base/algorithm.h"
#include "base/basic_types.h"
#include "base/object_ptr.h"
#include "dns/DnsValidator.h"
#include "dns/XrayDnsGenerator.h"
#include "ui/DnsServerEditorDialog.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QStackedLayout>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ui/qt_object_factory.h"
#include "ui/widgets/pill_tabs.h"

#include <rpl/rpl.h>

namespace zarya {

namespace {

QString enumKey(int value)
{
    return QString::number(value);
}

} // namespace

DnsProfileDialog::DnsProfileDialog(const DnsProfile& profile, bool readOnly, QWidget* parent)
    : QDialog(parent)
    , m_profile(profile)
    , m_readOnly(readOnly)
{
    const bool locked = readOnly || profile.isBuiltIn;
    m_readOnly = locked;
    setWindowTitle(locked ? tr("View DNS Profile")
                          : tr("Edit DNS Profile"));
    resize(820, 560);

    auto* tabs = Ui::CreateChild<Ui::PillTabs>(
        this,
        std::vector<QString>{tr("General"), tr("Servers"), tr("Hosts"), tr("Advanced")});
    auto* pageHost = new QWidget(this);
    m_pageStack = new QStackedLayout(pageHost);
    m_pageStack->setContentsMargins(0, 0, 0, 0);

    auto* generalTab = new QWidget(pageHost);
    m_nameEdit = new ZaryaTextField(tr("Name"), generalTab);
    m_nameEdit->setText(profile.name);
    m_enabledCheck = new ZaryaCheckBox(tr("Enabled"), generalTab, profile.enabled);

    m_modeCombo = new ZaryaSelector(generalTab);
    m_modeCombo->setItems({
        {enumKey(static_cast<int>(DnsProfileMode::System)),
         dnsProfileModeDisplayString(DnsProfileMode::System)},
        {enumKey(static_cast<int>(DnsProfileMode::SecureRemote)),
         dnsProfileModeDisplayString(DnsProfileMode::SecureRemote)},
        {enumKey(static_cast<int>(DnsProfileMode::ChinaDirectGlobalRemote)),
         dnsProfileModeDisplayString(DnsProfileMode::ChinaDirectGlobalRemote)},
        {enumKey(static_cast<int>(DnsProfileMode::Custom)),
         dnsProfileModeDisplayString(DnsProfileMode::Custom)},
    }, enumKey(static_cast<int>(profile.mode)));

    m_queryStrategyCombo = new ZaryaSelector(generalTab);
    m_queryStrategyCombo->setItems({
        {enumKey(static_cast<int>(DnsQueryStrategy::UseSystemDefault)),
         dnsQueryStrategyDisplayString(DnsQueryStrategy::UseSystemDefault)},
        {enumKey(static_cast<int>(DnsQueryStrategy::UseIP)),
         dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIP)},
        {enumKey(static_cast<int>(DnsQueryStrategy::UseIPv4)),
         dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv4)},
        {enumKey(static_cast<int>(DnsQueryStrategy::UseIPv6)),
         dnsQueryStrategyDisplayString(DnsQueryStrategy::UseIPv6)},
    }, enumKey(static_cast<int>(profile.queryStrategy)));

    auto* generalLayout = new QVBoxLayout(generalTab);
    generalLayout->setContentsMargins(8, 8, 8, 8);
    generalLayout->setSpacing(10);
    generalLayout->addWidget(new ZaryaFormRow(tr("Name"), m_nameEdit, generalTab));
    generalLayout->addWidget(new ZaryaFormRow(tr("Mode"), m_modeCombo, generalTab));
    generalLayout->addWidget(
        new ZaryaFormRow(tr("Query strategy"), m_queryStrategyCombo, generalTab));
    generalLayout->addWidget(m_enabledCheck);
    generalLayout->addStretch();

    auto* serversTab = new QWidget(pageHost);
    m_serversTable = new QTableWidget(serversTab);
    m_serversTable->setColumnCount(6);
    m_serversTable->setHorizontalHeaderLabels(
        {tr("Enabled"), tr("Address"), tr("Kind"),
         tr("Domains"), tr("Expect IPs"), tr("Note")});
    m_serversTable->horizontalHeader()->setStretchLastSection(true);
    m_serversTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serversTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serversTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto* addServerButton = new ZaryaActionButton(tr("Add Server"), serversTab);
    auto* editServerButton = new ZaryaActionButton(tr("Edit Server"), serversTab);
    auto* deleteServerButton = new ZaryaActionButton(tr("Delete Server"), serversTab);
    auto* moveUpButton = new ZaryaActionButton(tr("Move Up"), serversTab);
    auto* moveDownButton = new ZaryaActionButton(tr("Move Down"), serversTab);
    connect(addServerButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onAddServer);
    connect(editServerButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onEditServer);
    connect(deleteServerButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onDeleteServer);
    connect(moveUpButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onMoveUp);
    connect(moveDownButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onMoveDown);

    auto* serverButtons = new QHBoxLayout;
    serverButtons->addWidget(addServerButton);
    serverButtons->addWidget(editServerButton);
    serverButtons->addWidget(deleteServerButton);
    serverButtons->addWidget(moveUpButton);
    serverButtons->addWidget(moveDownButton);
    serverButtons->addStretch();

    auto* serversLayout = new QVBoxLayout(serversTab);
    serversLayout->setContentsMargins(8, 8, 8, 8);
    serversLayout->setSpacing(10);
    m_emptyServers = new ZaryaBodyText(
        tr("No DNS servers yet. Add a server to this profile."), serversTab);
    serversLayout->addWidget(m_emptyServers);
    serversLayout->addWidget(m_serversTable);
    serversLayout->addLayout(serverButtons);

    auto* hostsTab = new QWidget(pageHost);
    m_hostsEdit = new ZaryaTextArea(
        tr("One mapping per line: domain=ip or domain:example.com=1.2.3.4"), hostsTab, 300);
    m_hostsEdit->setText(hostsToText(profile.hosts));
    auto* hostsLayout = new QVBoxLayout(hostsTab);
    hostsLayout->setContentsMargins(8, 8, 8, 8);
    hostsLayout->addWidget(new ZaryaBodyText(
        tr("One mapping per line: domain=ip or domain:example.com=1.2.3.4"), hostsTab));
    hostsLayout->addWidget(m_hostsEdit);

    auto* advancedTab = new QWidget(pageHost);
    m_disableCacheCheck = new ZaryaCheckBox(
        tr("Disable DNS cache"), advancedTab, profile.disableCache);
    m_disableFallbackCheck = new ZaryaCheckBox(
        tr("Disable fallback"), advancedTab, profile.disableFallback);
    m_disableFallbackIfMatchCheck = new ZaryaCheckBox(
        tr("Disable fallback if match"), advancedTab, profile.disableFallbackIfMatch);
    auto* advancedLayout = new QVBoxLayout(advancedTab);
    advancedLayout->setContentsMargins(8, 8, 8, 8);
    advancedLayout->setSpacing(10);
    advancedLayout->addWidget(m_disableCacheCheck);
    advancedLayout->addWidget(m_disableFallbackCheck);
    advancedLayout->addWidget(m_disableFallbackIfMatchCheck);
    advancedLayout->addStretch();
    m_pageStack->addWidget(generalTab);
    m_pageStack->addWidget(serversTab);
    m_pageStack->addWidget(hostsTab);
    m_pageStack->addWidget(advancedTab);
    tabs->activeIndexChanges() | rpl::on_next(
        [this](int index) { m_pageStack->setCurrentIndex(index); }, tabs->lifetime());

    auto* validateButton = new ZaryaActionButton(tr("Validate"), this);
    auto* previewButton = new ZaryaActionButton(tr("Preview DNS JSON"), this);
    connect(validateButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onValidate);
    connect(previewButton, &ZaryaActionButton::clicked, this, &DnsProfileDialog::onPreviewDnsJson);

    QWidget* actions = nullptr;
    if (locked) {
        auto* closeButton = new ZaryaActionButton(tr("Close"), this);
        connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::reject);
        actions = closeButton;
    } else {
        auto* actionRow = new ZaryaDialogActionRow(tr("Save"), tr("Cancel"), this);
        connect(actionRow, &ZaryaDialogActionRow::accepted, this, &QDialog::accept);
        connect(actionRow, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);
        actions = actionRow;
    }

    auto* footer = new QHBoxLayout;
    footer->addWidget(validateButton);
    footer->addWidget(previewButton);
    footer->addStretch();
    footer->addWidget(actions);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(tabs);
    layout->addWidget(pageHost, 1);
    layout->addLayout(footer);

    if (locked) {
        m_nameEdit->setReadOnly(true);
        m_modeCombo->setEnabled(false);
        m_enabledCheck->setEnabled(false);
        m_queryStrategyCombo->setEnabled(false);
        addServerButton->setEnabled(false);
        editServerButton->setEnabled(false);
        deleteServerButton->setEnabled(false);
        moveUpButton->setEnabled(false);
        moveDownButton->setEnabled(false);
        m_hostsEdit->setReadOnly(true);
        m_disableCacheCheck->setEnabled(false);
        m_disableFallbackCheck->setEnabled(false);
        m_disableFallbackIfMatchCheck->setEnabled(false);
    }
    refreshServersTable();
    m_nameEdit->setFocus(Qt::OtherFocusReason);
}

DnsProfile DnsProfileDialog::profile() const
{
    DnsProfile result = m_profile;
    result.name = m_nameEdit->text().trimmed();
    result.mode = static_cast<DnsProfileMode>(m_modeCombo->currentKey().toInt());
    result.enabled = m_enabledCheck->isChecked();
    result.queryStrategy = static_cast<DnsQueryStrategy>(
        m_queryStrategyCombo->currentKey().toInt());
    result.hosts = parseHostsText(m_hostsEdit->text());
    result.disableCache = m_disableCacheCheck->isChecked();
    result.disableFallback = m_disableFallbackCheck->isChecked();
    result.disableFallbackIfMatch = m_disableFallbackIfMatchCheck->isChecked();
    return result;
}

void DnsProfileDialog::refreshServersTable()
{
    m_serversTable->setRowCount(m_profile.servers.size());
    m_emptyServers->setVisible(m_profile.servers.isEmpty());
    m_serversTable->setVisible(!m_profile.servers.isEmpty());
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
        UiMessagePresenter::information(
            this, tr("DNS validation"), tr("No validation warnings."));
        return;
    }
    UiMessagePresenter::warning(
        this, tr("DNS validation"), warnings.join(QStringLiteral("\n")));
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
