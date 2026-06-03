#include "ui/SubscriptionManagerDialog.h"

#include "domain/ProfileSourceType.h"
#include "domain/Subscription.h"
#include "storage/ProfileStore.h"
#include "storage/SubscriptionStore.h"
#include "subscription/SubscriptionManager.h"
#include "ui/SubscriptionDialog.h"
#include "ui/models/SubscriptionTableModel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace zarya {

SubscriptionManagerDialog::SubscriptionManagerDialog(
    QWidget* parent, QVector<Subscription>& subscriptions, QVector<Profile>& profiles,
    SubscriptionManager& manager, SubscriptionStore& subscriptionStore,
    ProfileStore& profileStore, const std::function<void(const QString&)>& logCallback,
    const std::function<void()>& profilesChangedCallback)
    : QDialog(parent)
    , m_subscriptions(subscriptions)
    , m_profiles(profiles)
    , m_manager(manager)
    , m_subscriptionStore(subscriptionStore)
    , m_profileStore(profileStore)
    , m_logCallback(logCallback)
    , m_profilesChangedCallback(profilesChangedCallback)
{
    setWindowTitle(QStringLiteral("Subscriptions"));
    resize(900, 480);

    m_tableModel = new SubscriptionTableModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);
    refreshTable();

    auto* addButton = new QPushButton(QStringLiteral("Add"), this);
    auto* editButton = new QPushButton(QStringLiteral("Edit"), this);
    auto* deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    auto* updateButton = new QPushButton(QStringLiteral("Update"), this);
    auto* updateAllButton = new QPushButton(QStringLiteral("Update All"), this);
    auto* closeButton = new QPushButton(QStringLiteral("Close"), this);

    connect(addButton, &QPushButton::clicked, this, &SubscriptionManagerDialog::onAdd);
    connect(editButton, &QPushButton::clicked, this, &SubscriptionManagerDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &SubscriptionManagerDialog::onDelete);
    connect(updateButton, &QPushButton::clicked, this,
            &SubscriptionManagerDialog::onUpdateSelected);
    connect(updateAllButton, &QPushButton::clicked, this, &SubscriptionManagerDialog::onUpdateAll);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(addButton);
    buttons->addWidget(editButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(updateButton);
    buttons->addWidget(updateAllButton);
    buttons->addStretch();
    buttons->addWidget(closeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_tableView);
    layout->addLayout(buttons);
}

void SubscriptionManagerDialog::refreshTable()
{
    m_tableModel->setSubscriptions(m_subscriptions);
}

int SubscriptionManagerDialog::selectedRow() const
{
    const QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        return -1;
    }
    return selected.first().row();
}

bool SubscriptionManagerDialog::persistAll(QString* errorMessage)
{
    QString error;
    if (!m_subscriptionStore.save(m_subscriptions, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    if (!m_profileStore.save(m_profiles, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    return true;
}

void SubscriptionManagerDialog::notifyProfilesChanged()
{
    if (m_profilesChangedCallback) {
        m_profilesChangedCallback();
    }
}

void SubscriptionManagerDialog::onAdd()
{
    Subscription subscription = Subscription::createDefault();
    if (!SubscriptionDialog::editSubscription(this, subscription)) {
        return;
    }
    m_subscriptions.append(subscription);
    refreshTable();
    persistAll(nullptr);
}

void SubscriptionManagerDialog::onEdit()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Edit subscription"),
                                 QStringLiteral("Select a subscription first."));
        return;
    }

    Subscription subscription = m_tableModel->subscriptionAt(row);
    if (!SubscriptionDialog::editSubscription(this, subscription)) {
        return;
    }

    m_subscriptions[row] = subscription;
    refreshTable();
    persistAll(nullptr);
}

void SubscriptionManagerDialog::onDelete()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Delete subscription"),
                                 QStringLiteral("Select a subscription first."));
        return;
    }

    const Subscription subscription = m_tableModel->subscriptionAt(row);
    const auto answer = QMessageBox::question(
        this, QStringLiteral("Delete subscription"),
        QStringLiteral("Delete subscription \"%1\"?\n\n"
                       "Yes = delete subscription and imported profiles\n"
                       "No = delete subscription only (keep profiles as manual)\n"
                       "Cancel = abort")
            .arg(subscription.name),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);

    if (answer == QMessageBox::Cancel) {
        return;
    }

    if (answer == QMessageBox::Yes) {
        QVector<Profile> kept;
        kept.reserve(m_profiles.size());
        for (const Profile& profile : m_profiles) {
            if (profile.subscriptionId != subscription.id) {
                kept.append(profile);
            }
        }
        m_profiles = std::move(kept);
    } else {
        for (Profile& profile : m_profiles) {
            if (profile.subscriptionId == subscription.id) {
                profile.sourceType = ProfileSourceType::Manual;
                profile.subscriptionId.clear();
                profile.subscriptionName.clear();
                profile.deletedBySubscriptionUpdate = false;
            }
        }
    }

    m_subscriptions.removeAt(row);
    refreshTable();
    QString error;
    if (!persistAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
    }
    notifyProfilesChanged();
}

void SubscriptionManagerDialog::onUpdateSelected()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Update subscription"),
                                 QStringLiteral("Select a subscription first."));
        return;
    }

    const SubscriptionUpdateResult result = m_manager.updateSubscription(m_subscriptions[row], m_profiles);
    refreshTable();
    QString error;
    if (!persistAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
    }
    notifyProfilesChanged();

    if (!result.success) {
        QMessageBox::warning(this, QStringLiteral("Update failed"), result.errorMessage);
    }
}

void SubscriptionManagerDialog::onUpdateAll()
{
    const QVector<SubscriptionUpdateResult> results =
        m_manager.updateAll(m_subscriptions, m_profiles);
    refreshTable();
    QString error;
    if (!persistAll(&error)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), error);
    }
    notifyProfilesChanged();

    int failed = 0;
    for (const SubscriptionUpdateResult& result : results) {
        if (!result.success) {
            ++failed;
        }
    }
    if (failed > 0) {
        QMessageBox::warning(this, QStringLiteral("Update all"),
                             QStringLiteral("%1 subscription(s) failed to update.").arg(failed));
    }
}

} // namespace zarya
