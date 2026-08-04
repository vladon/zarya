#include "ui/DnsManagerDialog.h"

#include "base/algorithm.h"
#include "dns/DnsValidator.h"
#include "dns/XrayDnsGenerator.h"
#include "ui/DnsManagerAccessibility.h"
#include "ui/DnsProfileDialog.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QTableWidget>
#include <QVBoxLayout>

#include <QUuid>

namespace zarya {

DnsManagerDialog::DnsManagerDialog(DnsManager& manager,
                                     const std::function<void(const QString&)>& logCallback,
                                     QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("DNS Profiles"));
    resize(920, 480);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {tr("Name"), tr("Mode"), tr("Built-in"),
         tr("Servers"), tr("Query Strategy"), tr("Flags")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    auto* newButton = new ZaryaActionButton(tr("New"), this, ZaryaButtonRole::Primary);
    auto* editButton = new ZaryaActionButton(tr("Edit"), this);
    auto* duplicateButton = new ZaryaActionButton(tr("Duplicate"), this);
    auto* deleteButton = new ZaryaActionButton(
        tr("Delete"), this, ZaryaButtonRole::Destructive);
    auto* setActiveButton = new ZaryaActionButton(tr("Set Active"), this);
    auto* previewButton = new ZaryaActionButton(tr("Preview JSON"), this);
    auto* closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(newButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onNew);
    connect(editButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onEdit);
    connect(duplicateButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onDuplicate);
    connect(deleteButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onDelete);
    connect(setActiveButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onSetActive);
    connect(previewButton, &ZaryaActionButton::clicked, this, &DnsManagerDialog::onPreview);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(newButton);
    buttons->addWidget(editButton);
    buttons->addWidget(duplicateButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(setActiveButton);
    buttons->addWidget(previewButton);
    buttons->addStretch();
    buttons->addWidget(closeButton);

    m_emptyState = new ZaryaBodyText(
        tr("No DNS profiles are available. Create one to get started."), this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_emptyState);
    layout->addWidget(m_table);
    layout->addLayout(buttons);
    refreshTable();
    configureDnsManagerAccessibility(
        m_table,
        {newButton,
         editButton,
         duplicateButton,
         deleteButton,
         setActiveButton,
         previewButton,
         closeButton},
        tr("DNS Profiles"));
}

void DnsManagerDialog::refreshTable()
{
    const QVector<DnsProfile> profiles = m_manager.profiles();
    m_table->setRowCount(profiles.size());
    m_emptyState->setVisible(profiles.isEmpty());
    m_table->setVisible(!profiles.isEmpty());
    const QString activeId = m_manager.activeProfileId();

    const XrayDnsGenerator generator;
    for (int row = 0; row < profiles.size(); ++row) {
        const DnsProfile& profile = profiles.at(row);
        QString name = profile.name;
        if (profile.id == activeId) {
            name += tr(" (active)");
        }
        m_table->setItem(row, 0, new QTableWidgetItem(name));
        m_table->setItem(row, 1,
                         new QTableWidgetItem(dnsProfileModeDisplayString(profile.mode)));
        m_table->setItem(row, 2,
                         new QTableWidgetItem(profile.isBuiltIn ? tr("Yes")
                                                              : tr("No")));
        m_table->setItem(row, 3,
                         new QTableWidgetItem(QString::number(generator.enabledServerCount(profile))));
        m_table->setItem(
            row, 4, new QTableWidgetItem(dnsQueryStrategyDisplayString(profile.queryStrategy)));
        m_table->setItem(row, 5, new QTableWidgetItem(flagsText(profile)));
        m_table->item(row, 0)->setData(Qt::UserRole, profile.id);
    }
}

QString DnsManagerDialog::flagsText(const DnsProfile& profile) const
{
    QStringList flags;
    if (!profile.enabled) {
        flags.append(tr("disabled"));
    }
    if (profile.disableCache) {
        flags.append(tr("no-cache"));
    }
    if (profile.disableFallback) {
        flags.append(tr("no-fallback"));
    }
    if (profile.disableFallbackIfMatch) {
        flags.append(tr("no-fallback-if-match"));
    }
    return flags.isEmpty() ? QStringLiteral("—") : flags.join(QStringLiteral(", "));
}

DnsProfile DnsManagerDialog::selectedProfile() const
{
    const int row = selectedRow();
    if (row < 0) {
        return {};
    }
    const QString id = m_table->item(row, 0)->data(Qt::UserRole).toString();
    return m_manager.profileById(id);
}

int DnsManagerDialog::selectedRow() const
{
    return m_table->currentRow();
}

void DnsManagerDialog::onNew()
{
    DnsProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = tr("New DNS Profile");
    profile.mode = DnsProfileMode::Custom;
    profile.enabled = true;
    profile.queryStrategy = DnsQueryStrategy::UseIP;

    DnsProfileDialog dialog(profile, false, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QStringList warnings = DnsValidator::warnings(dialog.profile());
    if (!warnings.isEmpty()) {
        if (!UiMessagePresenter::confirm(
                this,
                tr("DNS validation warnings"),
                tr("%1\n\nSave anyway?").arg(warnings.join(QStringLiteral("\n"))),
                tr("Save anyway"))) {
            return;
        }
    }
    m_manager.upsertProfile(dialog.profile());
    refreshTable();
}

void DnsManagerDialog::onEdit()
{
    const DnsProfile selected = selectedProfile();
    if (selected.id.isEmpty()) {
        UiMessagePresenter::information(
            this, tr("DNS Profiles"), tr("Select a DNS profile."));
        return;
    }
    DnsProfileDialog dialog(selected, selected.isBuiltIn, this);
    if (dialog.exec() != QDialog::Accepted || selected.isBuiltIn) {
        return;
    }
    const QStringList warnings = DnsValidator::warnings(dialog.profile());
    if (!warnings.isEmpty()) {
        if (!UiMessagePresenter::confirm(
                this,
                tr("DNS validation warnings"),
                tr("%1\n\nSave anyway?").arg(warnings.join(QStringLiteral("\n"))),
                tr("Save anyway"))) {
            return;
        }
    }
    m_manager.upsertProfile(dialog.profile());
    refreshTable();
}

void DnsManagerDialog::onDuplicate()
{
    const DnsProfile selected = selectedProfile();
    if (selected.id.isEmpty()) {
        return;
    }
    QString error;
    const DnsProfile copy = m_manager.duplicateProfile(selected.id, &error);
    if (copy.id.isEmpty()) {
        UiMessagePresenter::warning(this, tr("Duplicate"), error);
        return;
    }
    refreshTable();
}

void DnsManagerDialog::onDelete()
{
    const DnsProfile selected = selectedProfile();
    if (selected.id.isEmpty()) {
        return;
    }
    if (!UiMessagePresenter::confirm(
            this,
            tr("Delete DNS profile"),
            tr("Delete DNS profile \"%1\"?").arg(selected.name),
            tr("Delete"),
            true)) {
        return;
    }
    QString error;
    if (!m_manager.removeProfile(selected.id, &error)) {
        UiMessagePresenter::warning(this, tr("Delete"), error);
        return;
    }
    refreshTable();
}

void DnsManagerDialog::onSetActive()
{
    const DnsProfile selected = selectedProfile();
    if (selected.id.isEmpty()) {
        return;
    }
    m_manager.setActiveProfileId(selected.id);
    if (m_logCallback) {
        m_logCallback(tr("DNS profile changed: %1").arg(selected.name));
    }
    emit activeProfileChanged(selected.name);
    refreshTable();
}

void DnsManagerDialog::onPreview()
{
    const DnsProfile selected = selectedProfile();
    if (selected.id.isEmpty()) {
        return;
    }
    const XrayDnsGenerator generator;
    const QJsonObject dns = generator.generate(selected);
    const QString json =
        dns.isEmpty()
            ? tr("(DNS section omitted)")
            : QString::fromUtf8(QJsonDocument(dns).toJson(QJsonDocument::Indented));
    RoutingJsonPreviewDialog preview(json, this);
    preview.setWindowTitle(tr("DNS JSON Preview"));
    preview.exec();
}

} // namespace zarya
