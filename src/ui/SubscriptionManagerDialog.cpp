#include "ui/SubscriptionManagerDialog.h"

#include "ui/SubscriptionManagerAccessibility.h"
#include "domain/ProfileSourceType.h"
#include "domain/Subscription.h"
#include "i18n/ZaryaTr.h"
#include "storage/ProfileStore.h"
#include "storage/SubscriptionStore.h"
#include "subscription/SubscriptionManager.h"
#include "ui/SubscriptionDialog.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/models/SubscriptionTableModel.h"

#include <QHBoxLayout>
#include <QHeaderView>
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
    setWindowTitle(tr("Subscriptions"));
    resize(900, 480);

    m_tableModel = new SubscriptionTableModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);

    auto* addButton = new ZaryaActionButton(tr("Add"), this);
    auto* editButton = new ZaryaActionButton(tr("Edit"), this);
    auto* deleteButton = new ZaryaActionButton(tr("Delete"), this);
    auto* updateButton = new ZaryaActionButton(tr("Update"), this);
    auto* updateAllButton = new ZaryaActionButton(tr("Update All"), this);
    auto* closeButton = new ZaryaActionButton(tr("Close"), this);

    connect(addButton, &ZaryaActionButton::clicked, this, &SubscriptionManagerDialog::onAdd);
    connect(editButton, &ZaryaActionButton::clicked, this, &SubscriptionManagerDialog::onEdit);
    connect(deleteButton, &ZaryaActionButton::clicked, this, &SubscriptionManagerDialog::onDelete);
    connect(updateButton, &ZaryaActionButton::clicked, this,
            &SubscriptionManagerDialog::onUpdateSelected);
    connect(updateAllButton, &ZaryaActionButton::clicked, this,
            &SubscriptionManagerDialog::onUpdateAll);
    connect(closeButton, &ZaryaActionButton::clicked, this, &QDialog::accept);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(addButton);
    buttons->addWidget(editButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(updateButton);
    buttons->addWidget(updateAllButton);
    buttons->addStretch();
    buttons->addWidget(closeButton);

    m_emptyState = new ZaryaBodyText(
        tr("No subscriptions yet. Add one to import and update profiles."), this);
    m_updateSummaryLabel = new ZaryaBodyText(tr("Last update: —"), this);
    refreshTable();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(12);
    layout->addWidget(m_emptyState);
    layout->addWidget(m_tableView);
    layout->addWidget(m_updateSummaryLabel);
    layout->addLayout(buttons);
    configureSubscriptionManagerAccessibility(
        m_tableView,
        {addButton, editButton, deleteButton, updateButton, updateAllButton, closeButton},
        tr("Subscriptions"));
}

void SubscriptionManagerDialog::showUpdateSummary(const SubscriptionUpdateStats& stats)
{
    if (!m_updateSummaryLabel) {
        return;
    }
    m_updateSummaryLabel->setText(
        tr("Last update — Added: %1, Updated: %2, Missing: %3, Skipped: %4")
            .arg(stats.addedProfiles)
            .arg(stats.updatedProfiles)
            .arg(stats.markedMissingProfiles)
            .arg(stats.skippedLines));
}

void SubscriptionManagerDialog::refreshTable()
{
    m_tableModel->setSubscriptions(m_subscriptions);
    m_emptyState->setVisible(m_subscriptions.isEmpty());
    m_tableView->setVisible(!m_subscriptions.isEmpty());
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
        UiMessagePresenter::information(
            this, tr("Edit subscription"), tr("Select a subscription first."));
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
        UiMessagePresenter::information(
            this, tr("Delete subscription"), tr("Select a subscription first."));
        return;
    }

    const Subscription subscription = m_tableModel->subscriptionAt(row);
    const QString answer = UiMessagePresenter::choose(
        this, tr("Delete subscription"),
        tr("Delete subscription \"%1\"?\n\n"
           "Choose whether to delete its imported profiles or keep them as manual profiles.")
            .arg(subscription.name),
        UiMessageTone::Warning,
        {
            {QStringLiteral("delete-all"), tr("Delete with profiles"),
             UiMessageActionRole::Destructive, false, false},
            {QStringLiteral("keep-profiles"), tr("Keep profiles"),
             UiMessageActionRole::Secondary, false, false},
            {QStringLiteral("cancel"), tr("Cancel"),
             UiMessageActionRole::Secondary, true, true},
        });

    if (answer.isEmpty() || answer == QStringLiteral("cancel")) {
        return;
    }

    if (answer == QStringLiteral("delete-all")) {
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
        UiMessagePresenter::warning(this, tr("Save failed"), error);
    }
    notifyProfilesChanged();
}

void SubscriptionManagerDialog::onUpdateSelected()
{
    const int row = selectedRow();
    if (row < 0) {
        UiMessagePresenter::information(
            this, tr("Update subscription"), tr("Select a subscription first."));
        return;
    }

    m_updateSummaryLabel->setText(tr("Updating subscription…"));
    const SubscriptionUpdateResult result = m_manager.updateSubscription(m_subscriptions[row], m_profiles);
    refreshTable();
    QString error;
    if (!persistAll(&error)) {
        UiMessagePresenter::warning(this, tr("Save failed"), error);
    }
    notifyProfilesChanged();

    showUpdateSummary(result.stats);
    if (!result.success) {
        UiMessagePresenter::warning(this, tr("Update failed"), result.errorMessage);
    }
}

void SubscriptionManagerDialog::onUpdateAll()
{
    m_updateSummaryLabel->setText(tr("Updating all subscriptions…"));
    const QVector<SubscriptionUpdateResult> results =
        m_manager.updateAll(m_subscriptions, m_profiles);
    refreshTable();
    QString error;
    if (!persistAll(&error)) {
        UiMessagePresenter::warning(this, tr("Save failed"), error);
    }
    notifyProfilesChanged();

    SubscriptionUpdateStats total;
    int failed = 0;
    for (const SubscriptionUpdateResult& result : results) {
        total.addedProfiles += result.stats.addedProfiles;
        total.updatedProfiles += result.stats.updatedProfiles;
        total.markedMissingProfiles += result.stats.markedMissingProfiles;
        total.skippedLines += result.stats.skippedLines;
        if (!result.success) {
            ++failed;
        }
    }
    showUpdateSummary(total);
    if (failed > 0) {
        UiMessagePresenter::warning(
            this,
            tr("Update all"),
            ZaryaTr::plural("%n subscription(s) failed to update.", failed));
    }
}

} // namespace zarya
