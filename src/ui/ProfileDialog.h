#pragma once

#include "domain/Profile.h"

#include <QDialog>

class QKeyEvent;
class QStackedLayout;
class QVBoxLayout;
class QWidget;

namespace zarya {

class ZaryaCheckBox;
class ZaryaDialogActionRow;
class ZaryaFormRow;
class ZaryaNumberField;
class ZaryaSelector;
class ZaryaTextField;
class ZaryaValidationMessage;

class ProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileDialog(QWidget* parent = nullptr);

    void setProfile(const Profile& profile);
    Profile profile() const;

    static bool editProfile(QWidget* parent, Profile& profile);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QWidget* createPage(QVBoxLayout** layout);
    ZaryaFormRow* addRow(QVBoxLayout* layout, const QString& label, QWidget* field);
    bool validateInput(QString* errorMessage) const;
    void tryAccept();
    void populateFromProfile(const Profile& profile);
    Profile profileFromFields() const;
    void updateRealityTabVisibility();
    void updateProtocolFieldsVisibility();

    QWidget* m_tabs = nullptr; // Ui::PillTabs*
    QStackedLayout* m_pageStack = nullptr;

    ZaryaTextField* m_nameEdit = nullptr;
    ZaryaSelector* m_protocolCombo = nullptr;
    ZaryaSelector* m_coreCombo = nullptr;
    ZaryaTextField* m_addressEdit = nullptr;
    ZaryaNumberField* m_portSpin = nullptr;
    ZaryaTextField* m_uuidEdit = nullptr;
    ZaryaTextField* m_passwordEdit = nullptr;
    ZaryaTextField* m_encryptionEdit = nullptr;
    ZaryaTextField* m_methodEdit = nullptr;
    ZaryaTextField* m_securityCipherEdit = nullptr;
    ZaryaNumberField* m_alterIdSpin = nullptr;
    ZaryaCheckBox* m_enabledCheck = nullptr;
    ZaryaTextField* m_wgPeerPublicKeyEdit = nullptr;
    ZaryaTextField* m_localAddressEdit = nullptr;
    ZaryaNumberField* m_mtuSpin = nullptr;

    ZaryaSelector* m_networkCombo = nullptr;
    ZaryaTextField* m_pathEdit = nullptr;
    ZaryaTextField* m_hostEdit = nullptr;
    ZaryaTextField* m_headerTypeEdit = nullptr;
    ZaryaTextField* m_serviceNameEdit = nullptr;

    ZaryaSelector* m_securityCombo = nullptr;
    ZaryaTextField* m_serverNameEdit = nullptr;
    ZaryaTextField* m_publicKeyEdit = nullptr;
    ZaryaTextField* m_shortIdEdit = nullptr;
    ZaryaTextField* m_fingerprintEdit = nullptr;
    ZaryaTextField* m_spiderXEdit = nullptr;

    ZaryaTextField* m_flowEdit = nullptr;
    ZaryaTextField* m_sniEdit = nullptr;
    ZaryaTextField* m_remarkEdit = nullptr;
    ZaryaCheckBox* m_allowInsecureCheck = nullptr;
    ZaryaValidationMessage* m_unsupportedReasonLabel = nullptr;
    ZaryaValidationMessage* m_validationMessage = nullptr;
    ZaryaDialogActionRow* m_actions = nullptr;

    ZaryaFormRow* m_uuidRowWidget = nullptr;
    ZaryaFormRow* m_passwordRowWidget = nullptr;
    ZaryaFormRow* m_wgPeerPublicKeyRowWidget = nullptr;
    ZaryaFormRow* m_localAddressRowWidget = nullptr;
    ZaryaFormRow* m_mtuRowWidget = nullptr;
    ZaryaFormRow* m_methodRowWidget = nullptr;
    ZaryaFormRow* m_alterIdRowWidget = nullptr;
    ZaryaFormRow* m_securityCipherRowWidget = nullptr;
    ZaryaFormRow* m_encryptionRowWidget = nullptr;
    ZaryaFormRow* m_unsupportedReasonRow = nullptr;
    QString m_profileId;
};

} // namespace zarya
