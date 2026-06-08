#pragma once

#include <QDialog>
#include <functional>

class QLabel;
class QTableView;

namespace zarya {

struct Profile;
struct Subscription;
struct SubscriptionUpdateStats;
class SubscriptionManager;
class SubscriptionStore;
class ProfileStore;
class SubscriptionTableModel;

class SubscriptionManagerDialog : public QDialog {
    Q_OBJECT

public:
    SubscriptionManagerDialog(QWidget* parent, QVector<Subscription>& subscriptions,
                              QVector<Profile>& profiles, SubscriptionManager& manager,
                              SubscriptionStore& subscriptionStore, ProfileStore& profileStore,
                              const std::function<void(const QString&)>& logCallback,
                              const std::function<void()>& profilesChangedCallback);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onUpdateSelected();
    void onUpdateAll();

private:
    int selectedRow() const;
    void refreshTable();
    bool persistAll(QString* errorMessage);
    void notifyProfilesChanged();

    QVector<Subscription>& m_subscriptions;
    QVector<Profile>& m_profiles;
    SubscriptionManager& m_manager;
    SubscriptionStore& m_subscriptionStore;
    ProfileStore& m_profileStore;
    std::function<void(const QString&)> m_logCallback;
    std::function<void()> m_profilesChangedCallback;

    SubscriptionTableModel* m_tableModel = nullptr;
    QTableView* m_tableView = nullptr;
    QLabel* m_updateSummaryLabel = nullptr;

    void showUpdateSummary(const SubscriptionUpdateStats& stats);
};

} // namespace zarya
