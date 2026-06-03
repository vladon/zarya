#pragma once

#include "domain/Subscription.h"

#include <QAbstractTableModel>
#include <QVector>

namespace zarya {

class SubscriptionTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Name = 0,
        Url,
        Enabled,
        Profiles,
        LastUpdated,
        Status,
        LastError,
        ColumnCount,
    };

    explicit SubscriptionTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setSubscriptions(const QVector<Subscription>& subscriptions);
    const Subscription& subscriptionAt(int row) const;
    int rowForSubscriptionId(const QString& id) const;

private:
    QVector<Subscription> m_subscriptions;
};

} // namespace zarya
