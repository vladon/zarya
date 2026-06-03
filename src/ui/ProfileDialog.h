#pragma once

#include "domain/Profile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace zarya {

class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileDialog(QWidget* parent = nullptr);

    void setProfile(const Profile& profile);
    Profile profile() const;

    static bool editProfile(QWidget* parent, Profile& profile);

private:
    bool validateInput(QString* errorMessage) const;
    void populateFromProfile(const Profile& profile);
    Profile profileFromFields() const;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_protocolCombo = nullptr;
    QComboBox* m_coreCombo = nullptr;
    QLineEdit* m_addressEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLineEdit* m_uuidEdit = nullptr;
    QLineEdit* m_securityEdit = nullptr;
    QLineEdit* m_networkEdit = nullptr;
    QLineEdit* m_sniEdit = nullptr;
    QLineEdit* m_flowEdit = nullptr;
    QLineEdit* m_remarkEdit = nullptr;
    QCheckBox* m_enabledCheck = nullptr;
    QString m_profileId;
};

} // namespace zarya
