#include "ui/ProfileDialog.h"

#include "domain/CoreType.h"
#include "domain/ProfileValidation.h"
#include "domain/ProtocolType.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
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
    m_passwordEdit = new QLineEdit(basicPage);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_encryptionEdit = new QLineEdit(basicPage);
    m_encryptionEdit->setPlaceholderText(QStringLiteral("none"));
    m_methodEdit = new QLineEdit(basicPage);
    m_methodEdit->setPlaceholderText(QStringLiteral("2022-blake3-aes-128-gcm"));
    m_securityCipherEdit = new QLineEdit(basicPage);
    m_securityCipherEdit->setPlaceholderText(QStringLiteral("auto"));
    m_alterIdSpin = new QSpinBox(basicPage);
    m_alterIdSpin->setRange(0, 65535);
    m_enabledCheck = new QCheckBox(QStringLiteral("Enabled"), basicPage);
    m_enabledCheck->setChecked(true);
    m_unsupportedReasonLabel = new QLabel(basicPage);
    m_unsupportedReasonLabel->setWordWrap(true);
    m_unsupportedReasonLabel->setStyleSheet(QStringLiteral("color: #a63;"));

    connect(m_protocolCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ProfileDialog::onProtocolChanged);

    basicForm->addRow(QStringLiteral("Name"), m_nameEdit);
    basicForm->addRow(QStringLiteral("Protocol"), m_protocolCombo);
    basicForm->addRow(QStringLiteral("Core"), m_coreCombo);
    basicForm->addRow(QStringLiteral("Address"), m_addressEdit);
    basicForm->addRow(QStringLiteral("Port"), m_portSpin);
    m_uuidRowWidget = new QWidget(basicPage);
    auto* uuidRowLayout = new QFormLayout(m_uuidRowWidget);
    uuidRowLayout->setContentsMargins(0, 0, 0, 0);
    uuidRowLayout->addRow(QStringLiteral("UUID"), m_uuidEdit);
    basicForm->addRow(m_uuidRowWidget);
    m_passwordRowWidget = new QWidget(basicPage);
    auto* passwordRowLayout = new QFormLayout(m_passwordRowWidget);
    passwordRowLayout->setContentsMargins(0, 0, 0, 0);
    passwordRowLayout->addRow(QStringLiteral("Password"), m_passwordEdit);
    basicForm->addRow(m_passwordRowWidget);
    m_encryptionRowWidget = new QWidget(basicPage);
    auto* encryptionRowLayout = new QFormLayout(m_encryptionRowWidget);
    encryptionRowLayout->setContentsMargins(0, 0, 0, 0);
    encryptionRowLayout->addRow(QStringLiteral("VLESS encryption"), m_encryptionEdit);
    basicForm->addRow(m_encryptionRowWidget);
    m_methodRowWidget = new QWidget(basicPage);
    auto* methodRowLayout = new QFormLayout(m_methodRowWidget);
    methodRowLayout->setContentsMargins(0, 0, 0, 0);
    methodRowLayout->addRow(QStringLiteral("Method"), m_methodEdit);
    basicForm->addRow(m_methodRowWidget);
    m_alterIdRowWidget = new QWidget(basicPage);
    auto* alterRowLayout = new QFormLayout(m_alterIdRowWidget);
    alterRowLayout->setContentsMargins(0, 0, 0, 0);
    alterRowLayout->addRow(QStringLiteral("Alter ID"), m_alterIdSpin);
    basicForm->addRow(m_alterIdRowWidget);
    m_securityCipherRowWidget = new QWidget(basicPage);
    auto* cipherRowLayout = new QFormLayout(m_securityCipherRowWidget);
    cipherRowLayout->setContentsMargins(0, 0, 0, 0);
    cipherRowLayout->addRow(QStringLiteral("VMess security"), m_securityCipherEdit);
    basicForm->addRow(m_securityCipherRowWidget);
    basicForm->addRow(QStringLiteral("Import note"), m_unsupportedReasonLabel);
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
    m_serviceNameEdit = new QLineEdit(transportPage);
    transportForm->addRow(QStringLiteral("gRPC service"), m_serviceNameEdit);

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
    resize(560, 480);
    updateProtocolFieldsVisibility();
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
    m_tabs->setTabEnabled(m_tabs->indexOf(m_realityTab), true);
}

void ProfileDialog::onProtocolChanged(int index)
{
    Q_UNUSED(index);
    updateProtocolFieldsVisibility();
}

void ProfileDialog::updateProtocolFieldsVisibility()
{
    const auto protocol =
        static_cast<ProtocolType>(m_protocolCombo->currentData().toInt());

    m_uuidRowWidget->setVisible(protocol == ProtocolType::Vless
                                || protocol == ProtocolType::Vmess);
    m_passwordRowWidget->setVisible(protocol == ProtocolType::Trojan
                                    || protocol == ProtocolType::Shadowsocks
                                    || protocol == ProtocolType::Socks);
    m_encryptionRowWidget->setVisible(protocol == ProtocolType::Vless);
    m_methodRowWidget->setVisible(protocol == ProtocolType::Shadowsocks);
    m_alterIdRowWidget->setVisible(protocol == ProtocolType::Vmess);
    m_securityCipherRowWidget->setVisible(protocol == ProtocolType::Vmess);
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
    m_uuidEdit->setText(profile.effectiveUuid());
    m_passwordEdit->setText(profile.password.isEmpty() ? profile.uuidPassword : profile.password);
    m_encryptionEdit->setText(profile.encryption);
    m_methodEdit->setText(profile.effectiveMethod());
    m_securityCipherEdit->setText(profile.securityCipher.isEmpty() ? profile.effectiveVmessSecurity()
                                                                     : profile.securityCipher);
    m_alterIdSpin->setValue(profile.alterId);
    m_enabledCheck->setChecked(profile.enabled);
    if (profile.unsupportedReason.isEmpty()) {
        m_unsupportedReasonLabel->setText(QStringLiteral("—"));
    } else {
        m_unsupportedReasonLabel->setText(profile.unsupportedReason);
    }

    const int networkIndex = m_networkCombo->findData(profile.network);
    if (networkIndex >= 0) {
        m_networkCombo->setCurrentIndex(networkIndex);
    } else {
        m_networkCombo->setCurrentText(profile.network);
    }
    m_pathEdit->setText(profile.path);
    m_hostEdit->setText(profile.host);
    m_headerTypeEdit->setText(profile.headerType);
    m_serviceNameEdit->setText(profile.serviceName);

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
    updateProtocolFieldsVisibility();
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
    profile.password = m_passwordEdit->text().trimmed();
    profile.encryption = m_encryptionEdit->text().trimmed().isEmpty()
                             ? QStringLiteral("none")
                             : m_encryptionEdit->text().trimmed();
    profile.method = m_methodEdit->text().trimmed();
    profile.securityCipher = m_securityCipherEdit->text().trimmed();
    profile.alterId = m_alterIdSpin->value();
    profile.enabled = m_enabledCheck->isChecked();

    profile.network = m_networkCombo->currentData().toString();
    if (profile.network.isEmpty()) {
        profile.network = m_networkCombo->currentText().trimmed();
    }
    profile.path = m_pathEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.headerType = m_headerTypeEdit->text().trimmed();
    profile.serviceName = m_serviceNameEdit->text().trimmed();

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
