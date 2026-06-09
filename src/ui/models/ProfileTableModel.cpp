#include "ui/models/ProfileTableModel.h"

#include "domain/ProfileSourceType.h"
#include "testing/TestStatus.h"

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

QString ProfileTableModel::formatTcpColumn(const Profile& profile)
{
    if (profile.lastTestStatus == TestStatus::Testing) {
        return QStringLiteral("…");
    }
    if (profile.lastTcpPingMs >= 0) {
        return tr("%1 ms").arg(profile.lastTcpPingMs);
    }
    if (profile.lastTestStatus == TestStatus::Timeout && profile.lastRealDelayMs < 0) {
        return tr("timeout");
    }
    if (profile.lastTestStatus == TestStatus::Failed && profile.lastTcpPingMs < 0
        && profile.lastRealDelayMs < 0) {
        return tr("failed");
    }
    return {};
}

QString ProfileTableModel::formatDelayColumn(const Profile& profile)
{
    if (profile.lastTestStatus == TestStatus::Testing) {
        return QStringLiteral("…");
    }
    if (profile.lastRealDelayMs >= 0) {
        return tr("%1 ms").arg(profile.lastRealDelayMs);
    }
    return {};
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
                name += tr(" [missing]");
            }
            return name;
        }
        case Protocol:
            return protocolTypeToString(profile.protocol);
        case Address:
            return profile.address;
        case Port:
            return profile.port;
        case Tcp:
            return formatTcpColumn(profile);
        case Delay:
            return formatDelayColumn(profile);
        case TestStatus:
            return testStatusDisplayString(profile.lastTestStatus);
        case LastTested:
            return profile.lastTestedAt.isValid()
                       ? profile.lastTestedAt.toLocalTime().toString(Qt::ISODate)
                       : QString();
        case Core:
            return coreTypeToString(profile.coreType);
        case Source:
            return profileSourceTypeToString(profile.sourceType);
        case Subscription:
            return profile.subscriptionName;
        case Enabled:
            return profile.enabled ? tr("Yes") : tr("No");
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
        return tr("Name");
    case Protocol:
        return tr("Protocol");
    case Address:
        return tr("Address");
    case Port:
        return tr("Port");
    case Tcp:
        return QStringLiteral("TCP");
    case Delay:
        return tr("Delay");
    case TestStatus:
        return tr("Test Status");
    case LastTested:
        return tr("Last Tested");
    case Core:
        return tr("Core");
    case Source:
        return tr("Source");
    case Subscription:
        return tr("Subscription");
    case Enabled:
        return tr("Enabled");
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
