#include "ui/models/ProfileTableModel.h"

#include "domain/ProfileSourceType.h"

namespace zarya {

ProfileTableModel::ProfileTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ProfileTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_profiles.size();
}

int ProfileTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant ProfileTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size()) {
        return {};
    }

    const Profile& profile = m_profiles.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Name: {
            QString name = profile.name;
            if (profile.deletedBySubscriptionUpdate) {
                name += QStringLiteral(" [missing]");
            }
            return name;
        }
        case Protocol:
            return protocolTypeToString(profile.protocol);
        case Address:
            return profile.address;
        case Port:
            return profile.port;
        case Core:
            return coreTypeToString(profile.coreType);
        case Source:
            return profileSourceTypeToString(profile.sourceType);
        case Subscription:
            return profile.subscriptionName;
        case Enabled:
            return profile.enabled ? QStringLiteral("Yes") : QStringLiteral("No");
        default:
            break;
        }
    }
    return {};
}

QVariant ProfileTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case Name:
        return QStringLiteral("Name");
    case Protocol:
        return QStringLiteral("Protocol");
    case Address:
        return QStringLiteral("Address");
    case Port:
        return QStringLiteral("Port");
    case Core:
        return QStringLiteral("Core");
    case Source:
        return QStringLiteral("Source");
    case Subscription:
        return QStringLiteral("Subscription");
    case Enabled:
        return QStringLiteral("Enabled");
    default:
        return {};
    }
}

const Profile& ProfileTableModel::profileAt(int row) const
{
    return m_profiles.at(row);
}

Profile& ProfileTableModel::profileAt(int row)
{
    return m_profiles[row];
}

void ProfileTableModel::setProfiles(const QVector<Profile>& profiles)
{
    beginResetModel();
    m_profiles = profiles;
    endResetModel();
}

QVector<Profile> ProfileTableModel::profiles() const
{
    return m_profiles;
}

void ProfileTableModel::addProfile(const Profile& profile)
{
    const int row = m_profiles.size();
    beginInsertRows(QModelIndex(), row, row);
    m_profiles.append(profile);
    endInsertRows();
}

bool ProfileTableModel::updateProfile(int row, const Profile& profile)
{
    if (row < 0 || row >= m_profiles.size()) {
        return false;
    }
    m_profiles[row] = profile;
    const QModelIndex topLeft = index(row, 0);
    const QModelIndex bottomRight = index(row, ColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);
    return true;
}

bool ProfileTableModel::removeProfile(int row)
{
    if (row < 0 || row >= m_profiles.size()) {
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_profiles.removeAt(row);
    endRemoveRows();
    return true;
}

} // namespace zarya
