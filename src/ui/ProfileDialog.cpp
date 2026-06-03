#include "ui/ProfileDialog.h"

#include "domain/CoreType.h"
#include "domain/ProfileValidation.h"
#include "domain/ProtocolType.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QUuid>
#include <QVBoxLayout>

namespace zarya {

ProfileDialog::ProfileDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Profile"));

    m_nameEdit = new QLineEdit(this);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(QStringLiteral("VLESS"), static_cast<int>(ProtocolType::Vless));
    m_protocolCombo->addItem(QStringLiteral("VMess"), static_cast<int>(ProtocolType::Vmess));
    m_protocolCombo->addItem(QStringLiteral("Trojan"), static_cast<int>(ProtocolType::Trojan));
    m_protocolCombo->addItem(QStringLiteral("Shadowsocks"),
                             static_cast<int>(ProtocolType::Shadowsocks));
    m_protocolCombo->addItem(QStringLiteral("SOCKS"), static_cast<int>(ProtocolType::Socks));

    m_coreCombo = new QComboBox(this);
    m_coreCombo->addItem(QStringLiteral("Xray"), static_cast<int>(CoreType::Xray));
    m_coreCombo->addItem(QStringLiteral("SingBox"), static_cast<int>(CoreType::SingBox));

    m_addressEdit = new QLineEdit(this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(443);

    m_uuidEdit = new QLineEdit(this);
    m_securityCombo = new QComboBox(this);
    m_securityCombo->addItem(QStringLiteral("none"), QStringLiteral("none"));
    m_securityCombo->addItem(QStringLiteral("tls"), QStringLiteral("tls"));
    m_securityCombo->addItem(QStringLiteral("reality"), QStringLiteral("reality"));

    m_networkEdit = new QLineEdit(this);
    m_networkEdit->setPlaceholderText(QStringLiteral("tcp"));

    m_serverNameEdit = new QLineEdit(this);
    m_serverNameEdit->setPlaceholderText(QStringLiteral("REALITY / TLS server name"));

    m_sniEdit = new QLineEdit(this);
    m_sniEdit->setPlaceholderText(QStringLiteral("Legacy alias for server name"));

    m_flowEdit = new QLineEdit(this);
    m_flowEdit->setPlaceholderText(QStringLiteral("xtls-rprx-vision"));

    m_publicKeyEdit = new QLineEdit(this);
    m_publicKeyEdit->setPlaceholderText(QStringLiteral("REALITY public key (pbk)"));

    m_shortIdEdit = new QLineEdit(this);
    m_shortIdEdit->setPlaceholderText(QStringLiteral("Hex short ID (sid)"));

    m_fingerprintEdit = new QLineEdit(this);
    m_fingerprintEdit->setPlaceholderText(QStringLiteral("chrome"));

    m_spiderXEdit = new QLineEdit(this);
    m_spiderXEdit->setPlaceholderText(QStringLiteral("/"));

    m_remarkEdit = new QLineEdit(this);
    m_enabledCheck = new QCheckBox(QStringLiteral("Enabled"), this);
    m_enabledCheck->setChecked(true);

    auto* realityLayout = new QFormLayout;
    realityLayout->addRow(QStringLiteral("Public key"), m_publicKeyEdit);
    realityLayout->addRow(QStringLiteral("Short ID"), m_shortIdEdit);
    realityLayout->addRow(QStringLiteral("Fingerprint"), m_fingerprintEdit);
    realityLayout->addRow(QStringLiteral("Spider X"), m_spiderXEdit);

    auto* realityBox = new QGroupBox(QStringLiteral("REALITY"), this);
    realityBox->setLayout(realityLayout);
    m_realityGroup = realityBox;

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Name"), m_nameEdit);
    form->addRow(QStringLiteral("Protocol"), m_protocolCombo);
    form->addRow(QStringLiteral("Core"), m_coreCombo);
    form->addRow(QStringLiteral("Address"), m_addressEdit);
    form->addRow(QStringLiteral("Port"), m_portSpin);
    form->addRow(QStringLiteral("UUID / Password"), m_uuidEdit);
    form->addRow(QStringLiteral("Security"), m_securityCombo);
    form->addRow(QStringLiteral("Network"), m_networkEdit);
    form->addRow(QStringLiteral("Server name"), m_serverNameEdit);
    form->addRow(QStringLiteral("SNI (legacy)"), m_sniEdit);
    form->addRow(QStringLiteral("Flow"), m_flowEdit);
    form->addRow(QString(), m_realityGroup);
    form->addRow(QStringLiteral("Remark"), m_remarkEdit);
    form->addRow(QString(), m_enabledCheck);

    connect(m_securityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ProfileDialog::onSecurityChanged);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QString error;
        if (!validateInput(&error)) {
            QMessageBox::warning(this, QStringLiteral("Validation"), error);
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    resize(520, 560);

    updateRealityFieldVisibility();
}

void ProfileDialog::setProfile(const Profile& profile)
{
    populateFromProfile(profile);
}

Profile ProfileDialog::profile() const
{
    return profileFromFields();
}

bool ProfileDialog::editProfile(QWidget* parent, Profile& profile)
{
    ProfileDialog dialog(parent);
    dialog.setProfile(profile);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    profile = dialog.profile();
    return true;
}

bool ProfileDialog::validateInput(QString* errorMessage) const
{
    const Profile profile = profileFromFields();
    const ProfileValidationResult result = validateProfileForDialog(profile);
    if (!result.ok && errorMessage) {
        *errorMessage = result.message;
    }
    return result.ok;
}

void ProfileDialog::onSecurityChanged(int index)
{
    Q_UNUSED(index);
    updateRealityFieldVisibility();

    if (m_securityCombo->currentData().toString() == QStringLiteral("reality")
        && m_flowEdit->text().trimmed().isEmpty()) {
        m_flowEdit->setText(QStringLiteral("xtls-rprx-vision"));
    }
    if (m_securityCombo->currentData().toString() == QStringLiteral("reality")
        && m_networkEdit->text().trimmed().isEmpty()) {
        m_networkEdit->setText(QStringLiteral("tcp"));
    }
}

void ProfileDialog::updateRealityFieldVisibility()
{
    const bool reality =
        m_securityCombo->currentData().toString() == QStringLiteral("reality");
    m_realityGroup->setVisible(reality);
}

void ProfileDialog::populateFromProfile(const Profile& profile)
{
    m_profileId = profile.id;
    m_nameEdit->setText(profile.name);
    const int protocolIndex = m_protocolCombo->findData(static_cast<int>(profile.protocol));
    if (protocolIndex >= 0) {
        m_protocolCombo->setCurrentIndex(protocolIndex);
    }
    const int coreIndex = m_coreCombo->findData(static_cast<int>(profile.coreType));
    if (coreIndex >= 0) {
        m_coreCombo->setCurrentIndex(coreIndex);
    }
    m_addressEdit->setText(profile.address);
    m_portSpin->setValue(profile.port);
    m_uuidEdit->setText(profile.uuidPassword);

    QString security = profile.security.trimmed().toLower();
    if (security.isEmpty()) {
        security = QStringLiteral("none");
    }
    const int securityIndex = m_securityCombo->findData(security);
    if (securityIndex >= 0) {
        m_securityCombo->setCurrentIndex(securityIndex);
    } else {
        m_securityCombo->setCurrentIndex(0);
    }

    m_networkEdit->setText(profile.network);
    m_serverNameEdit->setText(profile.serverName);
    m_sniEdit->setText(profile.sni);
    m_flowEdit->setText(profile.flow);
    m_publicKeyEdit->setText(profile.publicKey);
    m_shortIdEdit->setText(profile.shortId);
    m_fingerprintEdit->setText(profile.fingerprint);
    m_spiderXEdit->setText(profile.spiderX);
    m_remarkEdit->setText(profile.remark);
    m_enabledCheck->setChecked(profile.enabled);
    updateRealityFieldVisibility();
}

Profile ProfileDialog::profileFromFields() const
{
    Profile profile;
    profile.id = m_profileId.isEmpty()
                     ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                     : m_profileId;
    profile.name = m_nameEdit->text().trimmed();
    profile.protocol =
        static_cast<ProtocolType>(m_protocolCombo->currentData().toInt());
    profile.coreType = static_cast<CoreType>(m_coreCombo->currentData().toInt());
    profile.address = m_addressEdit->text().trimmed();
    profile.port = m_portSpin->value();
    profile.uuidPassword = m_uuidEdit->text().trimmed();
    profile.security = m_securityCombo->currentData().toString();
    profile.network = m_networkEdit->text().trimmed();
    profile.serverName = m_serverNameEdit->text().trimmed();
    profile.sni = m_sniEdit->text().trimmed();
    profile.flow = m_flowEdit->text().trimmed();
    profile.publicKey = m_publicKeyEdit->text().trimmed();
    profile.shortId = m_shortIdEdit->text().trimmed();
    profile.fingerprint = m_fingerprintEdit->text().trimmed();
    profile.spiderX = m_spiderXEdit->text().trimmed();
    profile.remark = m_remarkEdit->text().trimmed();
    profile.enabled = m_enabledCheck->isChecked();
    return profile;
}

} // namespace zarya
