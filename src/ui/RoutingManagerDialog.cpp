#include "ui/RoutingManagerDialog.h"

#include "base/algorithm.h"
#include "domain/RoutingMode.h"
#include "routing/XrayRoutingGenerator.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/RoutingProfileDialog.h"
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

RoutingManagerDialog::RoutingManagerDialog(RoutingManager& manager,
                                           const std::function<void(const QString&)>& logCallback,
                                           QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_logCallback(logCallback)
{
    setWindowTitle(tr("Routing Profiles"));
    resize(900, 480);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {tr("Name"), tr("Mode"), tr("Built-in"),
         tr("Rules"), tr("Domain strategy")});
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

    connect(newButton, &ZaryaActionButton::clicked, this, &RoutingManagerDialog::onNew);
    connect(editButton, &ZaryaActionButton::clicked, this, &RoutingManagerDialog::onEdit);
    connect(duplicateButton, &ZaryaActionButton::clicked, this,
            &RoutingManagerDialog::onDuplicate);
    connect(deleteButton, &ZaryaActionButton::clicked, this, &RoutingManagerDialog::onDelete);
    connect(setActiveButton, &ZaryaActionButton::clicked, this,
            &RoutingManagerDialog::onSetActive);
    connect(previewButton, &ZaryaActionButton::clicked, this,
            &RoutingManagerDialog::onPreview);
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
        tr("No routing profiles are available. Create one to get started."), this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_emptyState);
    layout->addWidget(m_table);
    layout->addLayout(buttons);
    refreshTable();
}

void RoutingManagerDialog::refreshTable()
{
    const QVector<RoutingProfile> profiles = m_manager.profiles();
    m_table->setRowCount(profiles.size());
    m_emptyState->setVisible(profiles.isEmpty());
    m_table->setVisible(!profiles.isEmpty());
    const QString activeId = m_manager.activeProfileId();

    for (int row = 0; row < profiles.size(); ++row) {
        const RoutingProfile& profile = profiles[row];
        QString name = profile.name;
        if (profile.id == activeId) {
            name += tr(" (active)");
        }
        m_table->setItem(row, 0, new QTableWidgetItem(name));
        m_table->setItem(row, 1,
                         new QTableWidgetItem(routingModeDisplayString(profile.mode)));
        m_table->setItem(row, 2,
                         new QTableWidgetItem(profile.isBuiltIn ? tr("Yes")
                                                                : tr("No")));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(profile.rules.size())));
        m_table->setItem(row, 4, new QTableWidgetItem(profile.domainStrategy));
        m_table->item(row, 0)->setData(Qt::UserRole, profile.id);
    }
}

RoutingProfile RoutingManagerDialog::selectedProfile() const
{
    const int row = selectedRow();
    if (row < 0) {
        return {};
    }
    const QString id = m_table->item(row, 0)->data(Qt::UserRole).toString();
    return m_manager.profileById(id);
}

int RoutingManagerDialog::selectedRow() const
{
    const QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first()->row();
}

void RoutingManagerDialog::onNew()
{
    RoutingProfile profile;
    profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile.name = tr("My Routing");
    profile.mode = RoutingMode::Custom;
    profile.domainStrategy = QStringLiteral("AsIs");
    profile.enabled = true;
    profile.isBuiltIn = false;

    RoutingProfileDialog dialog(profile, false, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (!m_manager.upsertProfile(dialog.profile())) {
        UiMessagePresenter::warning(
            this, tr("Routing"), tr("Failed to save routing profile."));
        return;
    }
    QString error;
    m_manager.save(&error);
    refreshTable();
}

void RoutingManagerDialog::onEdit()
{
    const RoutingProfile profile = selectedProfile();
    if (profile.id.isEmpty()) {
        UiMessagePresenter::information(
            this, tr("Routing"), tr("Select a routing profile."));
        return;
    }

    RoutingProfileDialog dialog(profile, profile.isBuiltIn, this);
    const int result = dialog.exec();
    if (result == 2) {
        onDuplicate();
        return;
    }
    if (result != QDialog::Accepted || profile.isBuiltIn) {
        return;
    }

    if (!m_manager.upsertProfile(dialog.profile())) {
        UiMessagePresenter::warning(
            this, tr("Routing"), tr("Failed to update routing profile."));
        return;
    }
    QString error;
    m_manager.save(&error);
    refreshTable();
}

void RoutingManagerDialog::onDuplicate()
{
    const RoutingProfile profile = selectedProfile();
    if (profile.id.isEmpty()) {
        return;
    }
    QString error;
    const RoutingProfile copy = m_manager.duplicateProfile(profile.id, &error);
    if (copy.id.isEmpty()) {
        UiMessagePresenter::warning(this, tr("Routing"), error);
        return;
    }
    m_manager.save(&error);
    refreshTable();

    RoutingProfileDialog dialog(copy, false, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_manager.upsertProfile(dialog.profile());
        m_manager.save(&error);
        refreshTable();
    }
}

void RoutingManagerDialog::onDelete()
{
    const RoutingProfile profile = selectedProfile();
    if (profile.id.isEmpty()) {
        return;
    }
    if (profile.isBuiltIn) {
        UiMessagePresenter::information(
            this, tr("Routing"), tr("Built-in routing profiles cannot be deleted."));
        return;
    }
    if (!UiMessagePresenter::confirm(
            this,
            tr("Delete routing profile"),
            tr("Delete routing profile \"%1\"?").arg(profile.name),
            tr("Delete"),
            true)) {
        return;
    }
    QString error;
    if (!m_manager.removeProfile(profile.id, &error)) {
        UiMessagePresenter::warning(this, tr("Routing"), error);
        return;
    }
    m_manager.save(&error);
    refreshTable();
    emit activeProfileChanged(m_manager.activeProfile().name);
}

void RoutingManagerDialog::onSetActive()
{
    const RoutingProfile profile = selectedProfile();
    if (profile.id.isEmpty()) {
        return;
    }
    if (!m_manager.setActiveProfileId(profile.id)) {
        return;
    }
    m_manager.save();
    if (m_logCallback) {
        m_logCallback(tr("Routing profile changed: %1").arg(profile.name));
    }
    refreshTable();
    emit activeProfileChanged(profile.name);
}

void RoutingManagerDialog::onPreview()
{
    const RoutingProfile profile = selectedProfile();
    if (profile.id.isEmpty()) {
        return;
    }
    if (m_logCallback) {
        m_logCallback(tr("Routing config preview requested"));
    }
    const XrayRoutingGenerator generator;
    const QJsonObject routing = generator.generate(profile);
    const QString json =
        QString::fromUtf8(QJsonDocument(routing).toJson(QJsonDocument::Indented));
    RoutingJsonPreviewDialog dialog(json, this);
    dialog.exec();
}

} // namespace zarya
