#include "ui/RoutingManagerDialog.h"

#include "domain/RoutingMode.h"
#include "routing/XrayRoutingGenerator.h"
#include "ui/RoutingJsonPreviewDialog.h"
#include "ui/RoutingProfileDialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
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
    refreshTable();

    auto* newButton = new QPushButton(tr("New"), this);
    auto* editButton = new QPushButton(tr("Edit"), this);
    auto* duplicateButton = new QPushButton(tr("Duplicate"), this);
    auto* deleteButton = new QPushButton(tr("Delete"), this);
    auto* setActiveButton = new QPushButton(tr("Set Active"), this);
    auto* previewButton = new QPushButton(tr("Preview JSON"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);

    connect(newButton, &QPushButton::clicked, this, &RoutingManagerDialog::onNew);
    connect(editButton, &QPushButton::clicked, this, &RoutingManagerDialog::onEdit);
    connect(duplicateButton, &QPushButton::clicked, this, &RoutingManagerDialog::onDuplicate);
    connect(deleteButton, &QPushButton::clicked, this, &RoutingManagerDialog::onDelete);
    connect(setActiveButton, &QPushButton::clicked, this, &RoutingManagerDialog::onSetActive);
    connect(previewButton, &QPushButton::clicked, this, &RoutingManagerDialog::onPreview);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

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

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_table);
    layout->addLayout(buttons);
}

void RoutingManagerDialog::refreshTable()
{
    const QVector<RoutingProfile> profiles = m_manager.profiles();
    m_table->setRowCount(profiles.size());
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
        QMessageBox::warning(this, tr("Routing"),
                             tr("Failed to save routing profile."));
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
        QMessageBox::information(this, tr("Routing"),
                                 tr("Select a routing profile."));
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
        QMessageBox::warning(this, tr("Routing"),
                             tr("Failed to update routing profile."));
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
        QMessageBox::warning(this, tr("Routing"), error);
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
        QMessageBox::information(this, tr("Routing"),
                                 tr("Built-in routing profiles cannot be deleted."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Delete routing profile"),
        tr("Delete routing profile \"%1\"?").arg(profile.name));
    if (answer != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!m_manager.removeProfile(profile.id, &error)) {
        QMessageBox::warning(this, tr("Routing"), error);
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
