#include "ui/models/SubscriptionTableModel.h"

namespace zarya {

SubscriptionTableModel::SubscriptionTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int SubscriptionTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_subscriptions.size();
}

int SubscriptionTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant SubscriptionTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_subscriptions.size()) {
        return {};
    }

    const Subscription& subscription = m_subscriptions.at(index.row());
    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (index.column()) {
    case Name:
        return subscription.name;
    case Url:
        return subscription.url;
    case Enabled:
        return subscription.enabled ? QStringLiteral("Yes") : QStringLiteral("No");
    case Profiles:
        return subscription.profileCount;
    case LastUpdated:
        return subscription.lastUpdatedAt.isValid()
                   ? subscription.lastUpdatedAt.toLocalTime().toString(Qt::ISODate)
                   : QStringLiteral("-");
    case Status:
        return subscriptionStatusToString(subscription.lastStatus);
    case LastError:
        return subscription.lastError;
    default:
        return {};
    }
}

QVariant SubscriptionTableModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case Name:
        return QStringLiteral("Name");
    case Url:
        return QStringLiteral("URL");
    case Enabled:
        return QStringLiteral("Enabled");
    case Profiles:
        return QStringLiteral("Profiles");
    case LastUpdated:
        return QStringLiteral("Last Updated");
    case Status:
        return QStringLiteral("Status");
    case LastError:
        return QStringLiteral("Last Error");
    default:
        return {};
    }
}

void SubscriptionTableModel::setSubscriptions(const QVector<Subscription>& subscriptions)
{
    beginResetModel();
    m_subscriptions = subscriptions;
    endResetModel();
}

const Subscription& SubscriptionTableModel::subscriptionAt(int row) const
{
    return m_subscriptions.at(row);
}

int SubscriptionTableModel::rowForSubscriptionId(const QString& id) const
{
    for (int i = 0; i < m_subscriptions.size(); ++i) {
        if (m_subscriptions.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

} // namespace zarya
