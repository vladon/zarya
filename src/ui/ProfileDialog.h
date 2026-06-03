#pragma once

#include "domain/Profile.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTabWidget;
class QWidget;

namespace zarya {

class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileDialog(QWidget* parent = nullptr);

    void setProfile(const Profile& profile);
    Profile profile() const;

    static bool editProfile(QWidget* parent, Profile& profile);

private slots:
    void onSecurityChanged(int index);
    void onProtocolChanged(int index);

private:
    bool validateInput(QString* errorMessage) const;
    void populateFromProfile(const Profile& profile);
    Profile profileFromFields() const;
    void updateRealityTabVisibility();
    void updateProtocolFieldsVisibility();

    QTabWidget* m_tabs = nullptr;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_protocolCombo = nullptr;
    QComboBox* m_coreCombo = nullptr;
    QLineEdit* m_addressEdit = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QLineEdit* m_uuidEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_encryptionEdit = nullptr;
    QLineEdit* m_methodEdit = nullptr;
    QLineEdit* m_securityCipherEdit = nullptr;
    QSpinBox* m_alterIdSpin = nullptr;
    QCheckBox* m_enabledCheck = nullptr;

    QComboBox* m_networkCombo = nullptr;
    QLineEdit* m_pathEdit = nullptr;
    QLineEdit* m_hostEdit = nullptr;
    QLineEdit* m_headerTypeEdit = nullptr;
    QLineEdit* m_serviceNameEdit = nullptr;

    QComboBox* m_securityCombo = nullptr;
    QLineEdit* m_serverNameEdit = nullptr;
    QLineEdit* m_publicKeyEdit = nullptr;
    QLineEdit* m_shortIdEdit = nullptr;
    QLineEdit* m_fingerprintEdit = nullptr;
    QLineEdit* m_spiderXEdit = nullptr;

    QLineEdit* m_flowEdit = nullptr;
    QLineEdit* m_sniEdit = nullptr;
    QLineEdit* m_remarkEdit = nullptr;
    QCheckBox* m_allowInsecureCheck = nullptr;
    QLabel* m_unsupportedReasonLabel = nullptr;

    QWidget* m_realityTab = nullptr;
    QWidget* m_uuidRowWidget = nullptr;
    QWidget* m_passwordRowWidget = nullptr;
    QWidget* m_methodRowWidget = nullptr;
    QWidget* m_alterIdRowWidget = nullptr;
    QWidget* m_securityCipherRowWidget = nullptr;
    QWidget* m_encryptionRowWidget = nullptr;
    QString m_profileId;
};

} // namespace zarya
