#include "ui/ProfileDialog.h"

#include "domain/CoreType.h"
#include "domain/ProfileValidation.h"
#include "domain/ProtocolType.h"
#include "ui/desktopapp/ZaryaControls.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"

#include "base/object_ptr.h"
#include "styles/style_layers.h"
#include "ui/abstract_button.h"
#include "ui/qt_object_factory.h"
#include "ui/rp_widget.h"
#include "ui/widgets/pill_tabs.h"
#include "ui/widgets/scroll_area.h"

#include <QKeyEvent>
#include <QStackedLayout>
#include <QUuid>
#include <QVBoxLayout>
#include <rpl/rpl.h>

namespace zarya {
namespace {

QString enumKey(int value)
{
    return QString::number(value);
}

int enumValue(const ZaryaSelector* selector)
{
    return selector->currentKey().toInt();
}

} // namespace

ProfileDialog::ProfileDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Profile"));
    setAccessibleName(tr("Profile"));
    setModal(true);

    const std::vector<QString> tabLabels = {
        tr("Basic"),
        tr("Transport"),
        tr("TLS / REALITY"),
        tr("Advanced"),
    };
    auto* tabs = Ui::CreateChild<Ui::PillTabs>(this, tabLabels);
    QVector<Ui::AbstractButton*> tabButtons;
    for (QWidget* child : tabs->findChildren<QWidget*>(
             QString(), Qt::FindDirectChildrenOnly)) {
        if (auto* button = dynamic_cast<Ui::AbstractButton*>(child)) {
            tabButtons.push_back(button);
        }
    }
    Q_ASSERT(tabButtons.size() == static_cast<qsizetype>(tabLabels.size()));
    for (qsizetype index = 0; index < tabButtons.size(); ++index) {
        tabButtons[index]->setAccessibleName(tabLabels[index]);
    }
    m_tabs = tabs;

    auto* pageHost = new QWidget(this);
    m_pageStack = new QStackedLayout(pageHost);
    m_pageStack->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout* basicForm = nullptr;
    auto* basicPage = createPage(&basicForm);
    m_nameEdit = new ZaryaTextField(tr("Name"), basicPage);
    m_protocolCombo = new ZaryaSelector(basicPage);
    m_protocolCombo->setItems({
        {enumKey(static_cast<int>(ProtocolType::Vless)), QStringLiteral("VLESS")},
        {enumKey(static_cast<int>(ProtocolType::Vmess)), QStringLiteral("VMess")},
        {enumKey(static_cast<int>(ProtocolType::Trojan)), QStringLiteral("Trojan")},
        {enumKey(static_cast<int>(ProtocolType::Shadowsocks)), QStringLiteral("Shadowsocks")},
        {enumKey(static_cast<int>(ProtocolType::Socks)), QStringLiteral("SOCKS")},
        {enumKey(static_cast<int>(ProtocolType::Hysteria2)), QStringLiteral("Hysteria2")},
        {enumKey(static_cast<int>(ProtocolType::WireGuard)), QStringLiteral("WireGuard")},
    });
    m_coreCombo = new ZaryaSelector(basicPage);
    m_coreCombo->setItems({
        {enumKey(static_cast<int>(CoreType::Xray)), QStringLiteral("Xray")},
        {enumKey(static_cast<int>(CoreType::SingBox)), QStringLiteral("SingBox")},
    });
    m_addressEdit = new ZaryaTextField(tr("Address"), basicPage);
    m_portSpin = new ZaryaNumberField(tr("Port"), 1, 65535, basicPage);
    m_portSpin->setValue(443);
    m_uuidEdit = new ZaryaTextField(QStringLiteral("UUID"), basicPage);
    m_passwordEdit = new ZaryaTextField(tr("Password"), basicPage, true);
    m_encryptionEdit = new ZaryaTextField(QStringLiteral("none"), basicPage);
    m_methodEdit = new ZaryaTextField(QStringLiteral("2022-blake3-aes-128-gcm"), basicPage);
    m_securityCipherEdit = new ZaryaTextField(QStringLiteral("auto"), basicPage);
    m_alterIdSpin = new ZaryaNumberField(tr("Alter ID"), 0, 65535, basicPage);
    m_enabledCheck = new ZaryaCheckBox(tr("Enabled"), basicPage, true);
    m_wgPeerPublicKeyEdit = new ZaryaTextField(tr("Peer public key"), basicPage);
    m_localAddressEdit = new ZaryaTextField(QStringLiteral("10.0.0.2/32"), basicPage);
    m_mtuSpin = new ZaryaNumberField(tr("Default"), 0, 9000, basicPage);
    m_mtuSpin->setSpecialValueText(tr("Default"));
    m_unsupportedReasonLabel = new ZaryaValidationMessage(basicPage);

    addRow(basicForm, tr("Name"), m_nameEdit);
    addRow(basicForm, tr("Protocol"), m_protocolCombo);
    addRow(basicForm, tr("Core"), m_coreCombo);
    addRow(basicForm, tr("Address"), m_addressEdit);
    addRow(basicForm, tr("Port"), m_portSpin);
    m_uuidRowWidget = addRow(basicForm, QStringLiteral("UUID"), m_uuidEdit);
    m_passwordRowWidget = addRow(basicForm, tr("Password"), m_passwordEdit);
    m_wgPeerPublicKeyRowWidget = addRow(
        basicForm,
        tr("Peer public key"),
        m_wgPeerPublicKeyEdit);
    m_localAddressRowWidget = addRow(basicForm, tr("Local address"), m_localAddressEdit);
    m_mtuRowWidget = addRow(basicForm, tr("MTU"), m_mtuSpin);
    m_encryptionRowWidget = addRow(basicForm, tr("VLESS encryption"), m_encryptionEdit);
    m_methodRowWidget = addRow(basicForm, tr("Method"), m_methodEdit);
    m_alterIdRowWidget = addRow(basicForm, tr("Alter ID"), m_alterIdSpin);
    m_securityCipherRowWidget = addRow(
        basicForm,
        tr("VMess security"),
        m_securityCipherEdit);
    m_unsupportedReasonRow = addRow(
        basicForm,
        tr("Import note"),
        m_unsupportedReasonLabel);
    basicForm->addWidget(m_enabledCheck);
    basicForm->addStretch();

    QVBoxLayout* transportForm = nullptr;
    auto* transportPage = createPage(&transportForm);
    m_networkCombo = new ZaryaSelector(transportPage);
    m_networkCombo->setItems({
        {QStringLiteral("tcp"), QStringLiteral("tcp")},
        {QStringLiteral("ws"), QStringLiteral("ws")},
        {QStringLiteral("grpc"), QStringLiteral("grpc")},
    });
    m_pathEdit = new ZaryaTextField(tr("Path"), transportPage);
    m_hostEdit = new ZaryaTextField(tr("Host"), transportPage);
    m_headerTypeEdit = new ZaryaTextField(tr("Header type"), transportPage);
    m_serviceNameEdit = new ZaryaTextField(tr("gRPC service"), transportPage);
    addRow(transportForm, tr("Network"), m_networkCombo);
    addRow(transportForm, tr("Path"), m_pathEdit);
    addRow(transportForm, tr("Host"), m_hostEdit);
    addRow(transportForm, tr("Header type"), m_headerTypeEdit);
    addRow(transportForm, tr("gRPC service"), m_serviceNameEdit);
    transportForm->addStretch();

    QVBoxLayout* realityForm = nullptr;
    auto* realityPage = createPage(&realityForm);
    m_securityCombo = new ZaryaSelector(realityPage);
    m_securityCombo->setItems({
        {QStringLiteral("none"), QStringLiteral("none")},
        {QStringLiteral("tls"), QStringLiteral("tls")},
        {QStringLiteral("reality"), QStringLiteral("reality")},
    });
    m_serverNameEdit = new ZaryaTextField(tr("Server name (SNI)"), realityPage);
    m_publicKeyEdit = new ZaryaTextField(tr("Public key"), realityPage);
    m_shortIdEdit = new ZaryaTextField(tr("Short ID"), realityPage);
    m_fingerprintEdit = new ZaryaTextField(QStringLiteral("chrome"), realityPage);
    m_spiderXEdit = new ZaryaTextField(QStringLiteral("/"), realityPage);
    addRow(realityForm, tr("Security"), m_securityCombo);
    addRow(realityForm, tr("Server name (SNI)"), m_serverNameEdit);
    addRow(realityForm, tr("Public key"), m_publicKeyEdit);
    addRow(realityForm, tr("Short ID"), m_shortIdEdit);
    addRow(realityForm, tr("Fingerprint"), m_fingerprintEdit);
    addRow(realityForm, QStringLiteral("SpiderX"), m_spiderXEdit);
    realityForm->addStretch();

    QVBoxLayout* advancedForm = nullptr;
    auto* advancedPage = createPage(&advancedForm);
    m_flowEdit = new ZaryaTextField(QStringLiteral("xtls-rprx-vision"), advancedPage);
    m_sniEdit = new ZaryaTextField(tr("Legacy SNI field"), advancedPage);
    m_remarkEdit = new ZaryaTextField(tr("Remark"), advancedPage);
    m_allowInsecureCheck = new ZaryaCheckBox(tr("Allow insecure TLS"), advancedPage);
    addRow(advancedForm, tr("Flow"), m_flowEdit);
    addRow(advancedForm, tr("Legacy SNI field"), m_sniEdit);
    addRow(advancedForm, tr("Remark"), m_remarkEdit);
    advancedForm->addWidget(m_allowInsecureCheck);
    advancedForm->addStretch();

    m_pageStack->addWidget(basicPage);
    m_pageStack->addWidget(transportPage);
    m_pageStack->addWidget(realityPage);
    m_pageStack->addWidget(advancedPage);

    tabs->activeIndexChanges() | rpl::on_next(
        [this](int index) { m_pageStack->setCurrentIndex(index); },
        tabs->lifetime());
    connect(
        m_protocolCombo,
        &ZaryaSelector::currentKeyChanged,
        this,
        [this] { updateProtocolFieldsVisibility(); });
    connect(
        m_securityCombo,
        &ZaryaSelector::currentKeyChanged,
        this,
        [this](const QString& security) {
            updateRealityTabVisibility();
            if (security == QStringLiteral("reality")) {
                m_networkCombo->setCurrentKey(QStringLiteral("tcp"));
                if (m_fingerprintEdit->text().trimmed().isEmpty()) {
                    m_fingerprintEdit->setText(QStringLiteral("chrome"));
                }
            }
        });

    m_validationMessage = new ZaryaValidationMessage(this);
    m_actions = new ZaryaDialogActionRow(
        tr("Save"),
        tr("Cancel"),
        this,
        ZaryaButtonRole::Primary);
    connect(m_actions, &ZaryaDialogActionRow::accepted, this, &ProfileDialog::tryAccept);
    connect(m_actions, &ZaryaDialogActionRow::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(tabs);
    layout->addWidget(pageHost, 1);
    layout->addWidget(m_validationMessage);
    layout->addWidget(m_actions);

    resize(680, 620);
    updateProtocolFieldsVisibility();
    m_nameEdit->setFocus(Qt::OtherFocusReason);
}

QWidget* ProfileDialog::createPage(QVBoxLayout** layout)
{
    auto* scroll = Ui::CreateChild<Ui::ScrollArea>(this, st::boxScroll);
    scroll->setWidgetResizable(true);
    auto content = object_ptr<Ui::RpWidget>(scroll);
    auto* raw = content.data();
    auto* pageLayout = new QVBoxLayout(raw);
    pageLayout->setContentsMargins(8, 8, 8, 8);
    pageLayout->setSpacing(8);
    scroll->setOwnedWidget(std::move(content));
    *layout = pageLayout;
    return scroll;
}

ZaryaFormRow* ProfileDialog::addRow(
    QVBoxLayout* layout,
    const QString& label,
    QWidget* field)
{
    auto* row = new ZaryaFormRow(label, field, layout->parentWidget());
    layout->addWidget(row);
    return row;
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

void ProfileDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && !(event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier))) {
        tryAccept();
        return;
    }
    QDialog::keyPressEvent(event);
}

bool ProfileDialog::validateInput(QString* errorMessage) const
{
    const ProfileValidationResult result = validateProfileForDialog(profileFromFields());
    if (!result.ok && errorMessage) {
        *errorMessage = result.message;
    }
    return result.ok;
}

void ProfileDialog::tryAccept()
{
    QString error;
    if (!validateInput(&error)) {
        m_validationMessage->showMessage(
            error,
            QAccessible::AnnouncementPoliteness::Assertive);
        const bool nameMissing = m_nameEdit->text().trimmed().isEmpty();
        const bool addressMissing = m_addressEdit->text().trimmed().isEmpty();
        m_nameEdit->showError(nameMissing);
        m_addressEdit->showError(addressMissing);
        if (nameMissing) {
            m_nameEdit->setFocus(Qt::OtherFocusReason);
        } else if (addressMissing) {
            m_addressEdit->setFocus(Qt::OtherFocusReason);
        }
        return;
    }
    m_validationMessage->clear();
    m_nameEdit->showError(false);
    m_addressEdit->showError(false);
    accept();
}

void ProfileDialog::updateRealityTabVisibility()
{
    // TLS and REALITY fields remain available for every protocol, matching the old dialog.
}

void ProfileDialog::updateProtocolFieldsVisibility()
{
    const auto protocol = static_cast<ProtocolType>(enumValue(m_protocolCombo));
    m_uuidRowWidget->setVisible(
        protocol == ProtocolType::Vless || protocol == ProtocolType::Vmess);
    m_passwordRowWidget->setVisible(
        protocol == ProtocolType::Trojan
        || protocol == ProtocolType::Shadowsocks
        || protocol == ProtocolType::Socks
        || protocol == ProtocolType::Hysteria2
        || protocol == ProtocolType::WireGuard);
    m_passwordRowWidget->setLabel(
        protocol == ProtocolType::WireGuard ? tr("Private key") : tr("Password"));
    const bool wireGuard = protocol == ProtocolType::WireGuard;
    m_wgPeerPublicKeyRowWidget->setVisible(wireGuard);
    m_localAddressRowWidget->setVisible(wireGuard);
    m_mtuRowWidget->setVisible(wireGuard);
    m_encryptionRowWidget->setVisible(protocol == ProtocolType::Vless);
    m_methodRowWidget->setVisible(protocol == ProtocolType::Shadowsocks);
    m_alterIdRowWidget->setVisible(protocol == ProtocolType::Vmess);
    m_securityCipherRowWidget->setVisible(protocol == ProtocolType::Vmess);
}

void ProfileDialog::populateFromProfile(const Profile& profile)
{
    m_profileId = profile.id;
    m_nameEdit->setText(profile.name);
    m_protocolCombo->setCurrentKey(enumKey(static_cast<int>(profile.protocol)));
    m_coreCombo->setCurrentKey(enumKey(static_cast<int>(profile.coreType)));
    m_addressEdit->setText(profile.address);
    m_portSpin->setValue(profile.port);
    m_uuidEdit->setText(profile.effectiveUuid());
    m_passwordEdit->setText(
        profile.password.isEmpty() ? profile.uuidPassword : profile.password);
    m_encryptionEdit->setText(profile.encryption);
    m_methodEdit->setText(profile.effectiveMethod());
    m_securityCipherEdit->setText(
        profile.securityCipher.isEmpty()
            ? profile.effectiveVmessSecurity()
            : profile.securityCipher);
    m_alterIdSpin->setValue(profile.alterId);
    m_enabledCheck->setChecked(profile.enabled);
    if (profile.unsupportedReason.isEmpty()) {
        m_unsupportedReasonLabel->clear();
        m_unsupportedReasonRow->hide();
    } else {
        m_unsupportedReasonLabel->showMessage(profile.unsupportedReason);
        m_unsupportedReasonRow->show();
    }

    if (!m_networkCombo->setCurrentKey(profile.network)) {
        m_networkCombo->setCurrentKey(QStringLiteral("tcp"));
    }
    m_pathEdit->setText(profile.path);
    m_hostEdit->setText(profile.host);
    m_headerTypeEdit->setText(profile.headerType);
    m_serviceNameEdit->setText(profile.serviceName);

    QString security = profile.security.trimmed().toLower();
    if (security.isEmpty()) {
        security = QStringLiteral("none");
    }
    m_securityCombo->setCurrentKey(security);
    m_serverNameEdit->setText(
        profile.serverName.isEmpty() ? profile.sni : profile.serverName);
    m_publicKeyEdit->setText(profile.publicKey);
    m_wgPeerPublicKeyEdit->setText(profile.publicKey);
    m_localAddressEdit->setText(profile.localAddress);
    m_mtuSpin->setValue(profile.mtu > 0 ? profile.mtu : 0);
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
    profile.protocol = static_cast<ProtocolType>(enumValue(m_protocolCombo));
    profile.coreType = static_cast<CoreType>(enumValue(m_coreCombo));
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
    profile.network = m_networkCombo->currentKey();
    profile.path = m_pathEdit->text().trimmed();
    profile.host = m_hostEdit->text().trimmed();
    profile.headerType = m_headerTypeEdit->text().trimmed();
    profile.serviceName = m_serviceNameEdit->text().trimmed();
    profile.security = m_securityCombo->currentKey();
    profile.serverName = m_serverNameEdit->text().trimmed();
    profile.sni = m_sniEdit->text().trimmed();
    if (profile.protocol == ProtocolType::WireGuard) {
        profile.publicKey = m_wgPeerPublicKeyEdit->text().trimmed();
        profile.localAddress = m_localAddressEdit->text().trimmed();
        profile.mtu = m_mtuSpin->value();
    } else {
        profile.publicKey = m_publicKeyEdit->text().trimmed();
    }
    profile.shortId = m_shortIdEdit->text().trimmed();
    profile.fingerprint = m_fingerprintEdit->text().trimmed();
    profile.spiderX = m_spiderXEdit->text().trimmed();
    profile.flow = m_flowEdit->text().trimmed();
    profile.remark = m_remarkEdit->text().trimmed();
    profile.allowInsecure = m_allowInsecureCheck->isChecked();
    return profile;
}

} // namespace zarya
