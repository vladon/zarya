#include "ui/ProfileDialog.h"

#include "domain/CoreType.h"
#include "domain/ProfileValidation.h"
#include "domain/ProtocolType.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QUuid>
#include <QVBoxLayout>

namespace zarya {

ProfileDialog::ProfileDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Profile"));
    m_tabs = new QTabWidget(this);

    auto* basicPage = new QWidget(this);
    auto* basicForm = new QFormLayout(basicPage);
    m_nameEdit = new QLineEdit(basicPage);
    m_protocolCombo = new QComboBox(basicPage);
    m_protocolCombo->addItem(QStringLiteral("VLESS"), static_cast<int>(ProtocolType::Vless));
    m_protocolCombo->addItem(QStringLiteral("VMess"), static_cast<int>(ProtocolType::Vmess));
    m_protocolCombo->addItem(QStringLiteral("Trojan"), static_cast<int>(ProtocolType::Trojan));
    m_protocolCombo->addItem(QStringLiteral("Shadowsocks"),
                             static_cast<int>(ProtocolType::Shadowsocks));
    m_protocolCombo->addItem(QStringLiteral("SOCKS"), static_cast<int>(ProtocolType::Socks));

    m_coreCombo = new QComboBox(basicPage);
    m_coreCombo->addItem(QStringLiteral("Xray"), static_cast<int>(CoreType::Xray));
    m_coreCombo->addItem(QStringLiteral("SingBox"), static_cast<int>(CoreType::SingBox));

    m_addressEdit = new QLineEdit(basicPage);
    m_portSpin = new QSpinBox(basicPage);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(443);
    m_uuidEdit = new QLineEdit(basicPage);
    m_encryptionEdit = new QLineEdit(basicPage);
    m_encryptionEdit->setPlaceholderText(QStringLiteral("none"));
    m_enabledCheck = new QCheckBox(QStringLiteral("Enabled"), basicPage);
    m_enabledCheck->setChecked(true);

    basicForm->addRow(QStringLiteral("Name"), m_nameEdit);
    basicForm->addRow(QStringLiteral("Protocol"), m_protocolCombo);
    basicForm->addRow(QStringLiteral("Core"), m_coreCombo);
    basicForm->addRow(QStringLiteral("Address"), m_addressEdit);
    basicForm->addRow(QStringLiteral("Port"), m_portSpin);
    basicForm->addRow(QStringLiteral("UUID"), m_uuidEdit);
    basicForm->addRow(QStringLiteral("Encryption"), m_encryptionEdit);
    basicForm->addRow(QString(), m_enabledCheck);

    auto* transportPage = new QWidget(this);
    auto* transportForm = new QFormLayout(transportPage);
    m_networkCombo = new QComboBox(transportPage);
    m_networkCombo->addItem(QStringLiteral("tcp"), QStringLiteral("tcp"));
    m_networkCombo->addItem(QStringLiteral("ws"), QStringLiteral("ws"));
    m_networkCombo->addItem(QStringLiteral("grpc"), QStringLiteral("grpc"));
    m_pathEdit = new QLineEdit(transportPage);
    m_hostEdit = new QLineEdit(transportPage);
    m_headerTypeEdit = new QLineEdit(transportPage);
    transportForm->addRow(QStringLiteral("Network"), m_networkCombo);
    transportForm->addRow(QStringLiteral("Path"), m_pathEdit);
    transportForm->addRow(QStringLiteral("Host"), m_hostEdit);
    transportForm->addRow(QStringLiteral("Header type"), m_headerTypeEdit);

    m_realityTab = new QWidget(this);
    auto* realityForm = new QFormLayout(m_realityTab);
    m_securityCombo = new QComboBox(m_realityTab);
    m_securityCombo->addItem(QStringLiteral("none"), QStringLiteral("none"));
    m_securityCombo->addItem(QStringLiteral("tls"), QStringLiteral("tls"));
    m_securityCombo->addItem(QStringLiteral("reality"), QStringLiteral("reality"));
    connect(m_securityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ProfileDialog::onSecurityChanged);

    m_serverNameEdit = new QLineEdit(m_realityTab);
    m_publicKeyEdit = new QLineEdit(m_realityTab);
    m_shortIdEdit = new QLineEdit(m_realityTab);
    m_fingerprintEdit = new QLineEdit(m_realityTab);
    m_fingerprintEdit->setPlaceholderText(QStringLiteral("chrome"));
    m_spiderXEdit = new QLineEdit(m_realityTab);
    m_spiderXEdit->setPlaceholderText(QStringLiteral("/"));

    realityForm->addRow(QStringLiteral("Security"), m_securityCombo);
    realityForm->addRow(QStringLiteral("Server name (SNI)"), m_serverNameEdit);
    realityForm->addRow(QStringLiteral("Public key"), m_publicKeyEdit);
    realityForm->addRow(QStringLiteral("Short ID"), m_shortIdEdit);
    realityForm->addRow(QStringLiteral("Fingerprint"), m_fingerprintEdit);
    realityForm->addRow(QStringLiteral("SpiderX"), m_spiderXEdit);

    auto* advancedPage = new QWidget(this);
    auto* advancedForm = new QFormLayout(advancedPage);
    m_flowEdit = new QLineEdit(advancedPage);
    m_flowEdit->setPlaceholderText(QStringLiteral("xtls-rprx-vision"));
    m_sniEdit = new QLineEdit(advancedPage);
    m_remarkEdit = new QLineEdit(advancedPage);
    m_allowInsecureCheck = new QCheckBox(QStringLiteral("Allow insecure TLS"), advancedPage);
    advancedForm->addRow(QStringLiteral("Flow"), m_flowEdit);
    advancedForm->addRow(QStringLiteral("Legacy SNI field"), m_sniEdit);
    advancedForm->addRow(QStringLiteral("Remark"), m_remarkEdit);
    advancedForm->addRow(QString(), m_allowInsecureCheck);

    m_tabs->addTab(basicPage, QStringLiteral("Basic"));
    m_tabs->addTab(transportPage, QStringLiteral("Transport"));
    m_tabs->addTab(m_realityTab, QStringLiteral("TLS / REALITY"));
    m_tabs->addTab(advancedPage, QStringLiteral("Advanced"));

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
    layout->addWidget(m_tabs);
    layout->addWidget(buttons);
    resize(520, 420);
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
    updateRealityTabVisibility();
    if (m_securityCombo->currentData().toString() == QStringLiteral("reality")) {
        if (m_networkCombo->currentText() != QStringLiteral("tcp")) {
            m_networkCombo->setCurrentText(QStringLiteral("tcp"));
        }
        if (m_fingerprintEdit->text().trimmed().isEmpty()) {
            m_fingerprintEdit->setText(QStringLiteral("chrome"));
        }
    }
}

void ProfileDialog::updateRealityTabVisibility()
{
    const bool reality = m_securityCombo->currentData().toString() == QStringLiteral("reality");
    m_tabs->setTabEnabled(m_tabs->indexOf(m_realityTab), true);
    Q_UNUSED(reality);
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
    m_encryptionEdit->setText(profile.encryption);
    m_enabledCheck->setChecked(profile.enabled);

    const int networkIndex = m_networkCombo->findData(profile.network);
    if (networkIndex >= 0) {
        m_networkCombo->setCurrentIndex(networkIndex);
    } else {
        m_networkCombo->setCurrentText(profile.network);
    }
    m_pathEdit->setText(profile.path);
    m_hostEdit->setText(profile.host);
    m_headerTypeEdit->setText(profile.headerType);

    QString security = profile.security.trimmed().toLower();
    if (security.isEmpty()) {
        security = QStringLiteral("none");
    }
    const int securityIndex = m_securityCombo->findData(security);
    if (securityIndex >= 0) {
        m_securityCombo->setCurrentIndex(securityIndex);
    }

    m_serverNameEdit->setText(profile.serverName.isEmpty() ? profile.sni : profile.serverName);
    m_publicKeyEdit->setText(profile.publicKey);
    m_shortIdEdit->setText(profile.shortId);
    m_fingerprintEdit->setText(profile.fingerprint);
    m_spiderXEdit->setText(profile.spiderX);
    m_flowEdit->setText(profile.flow);
    m_sniEdit->setText(profile.sni);
    m_remarkEdit->setText(profile.remark);
    m_allowInsecureCheck->setChecked(profile.allowInsecure);
    updateRealityTabVisibility();
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
    profile.encryption = m_encryptionEdit->text().trimmed().isEmpty()
                             ? QStringLiteral("none")
                             : m_encryptionEdit->text().trimmed();
    profile.enabled = m_enabledCheck->isChecked();

    profile.network = m_networkCombo->currentData().toString();
    if (profile.network.isEmpty()) {
        profile.network = m_networkCombo->currentText().trimmed();
    }
    profile.path = m_pathEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.headerType = m_headerTypeEdit->text().trimmed();

    profile.security = m_securityCombo->currentData().toString();
    profile.serverName = m_serverNameEdit->text().trimmed();
    profile.sni = m_sniEdit->text().trimmed();
    profile.publicKey = m_publicKeyEdit->text().trimmed();
    profile.shortId = m_shortIdEdit->text().trimmed();
    profile.fingerprint = m_fingerprintEdit->text().trimmed();
    profile.spiderX = m_spiderXEdit->text().trimmed();
    profile.flow = m_flowEdit->text().trimmed();
    profile.remark = m_remarkEdit->text().trimmed();
    profile.allowInsecure = m_allowInsecureCheck->isChecked();
    return profile;
}

} // namespace zarya
