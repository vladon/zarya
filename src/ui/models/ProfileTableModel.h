#pragma once

#include "domain/Profile.h"

#include <QAbstractTableModel>
#include <QVector>

namespace zarya {

class ProfileTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Name = 0,
        Protocol,
        Address,
        Port,
        Core,
        Enabled,
        ColumnCount,
    };

    explicit ProfileTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const Profile& profileAt(int row) const;
    Profile& profileAt(int row);
    void setProfiles(const QVector<Profile>& profiles);
    QVector<Profile> profiles() const;

    void addProfile(const Profile& profile);
    bool updateProfile(int row, const Profile& profile);
    bool removeProfile(int row);

private:
    QVector<Profile> m_profiles;
};

} // namespace zarya
