#include "ui/onboarding/FirstRunWizard.h"

#include "cores/CoreBinaryManager.h"
#include "dns/DnsManager.h"
#include "domain/DnsProfileMode.h"
#include "domain/RoutingMode.h"
#include "routing/RoutingManager.h"
#include "ui/import/ProfileImportWidget.h"
#include "ui/onboarding/FirstRunChecklistWidget.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QUuid>
#include <QVBoxLayout>

namespace zarya {

namespace {

constexpr int kPageWelcome = 0;
constexpr int kPageCore = 1;
constexpr int kPageImport = 2;
constexpr int kPageRoutingDns = 3;
constexpr int kPageRuntime = 4;
constexpr int kPageFinish = 5;

} // namespace

FirstRunWizard::FirstRunWizard(CoreBinaryManager* coreManager, RoutingManager* routingManager,
                               DnsManager* dnsManager, QWidget* parent)
    : QWizard(parent)
    , m_coreManager(coreManager)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
{
    FirstRunState::applyDefaults(&m_state);
    setWindowTitle(tr("Zarya Setup"));
    setWizardStyle(QWizard::ModernStyle);
    resize(620, 480);
    setupPages();
}

FirstRunState FirstRunWizard::state() const
{
    return m_state;
}

bool FirstRunWizard::wasSkipped() const
{
    return m_skipped;
}

void FirstRunWizard::setupPages()
{
    // Welcome
    auto* welcome = new QWizardPage(this);
    welcome->setTitle(tr("Welcome to Zarya"));
    auto* welcomeText = new QLabel(
        tr("Zarya is a cross-platform proxy client.\n\n"
           "Recommended setup:\n"
           "1. Install Xray core\n"
           "2. Add a profile or subscription\n"
           "3. Choose routing/DNS behavior\n"
           "4. Start a profile"),
        welcome);
    welcomeText->setWordWrap(true);
    auto* welcomeLayout = new QVBoxLayout(welcome);
    welcomeLayout->addWidget(welcomeText);
    welcomeLayout->addStretch();
    setPage(kPageWelcome, welcome);

    // Core setup
    auto* corePage = new QWizardPage(this);
    corePage->setTitle(tr("Core setup"));
    auto* coreInfo = new QLabel(corePage);
    coreInfo->setObjectName(QStringLiteral("coreStatusLabel"));
    coreInfo->setWordWrap(true);
    auto* coreHelp = new QLabel(
        tr("Xray is required for the default system-proxy mode.\n"
           "sing-box is only needed for experimental TUN mode."),
        corePage);
    coreHelp->setWordWrap(true);
    auto* installXrayBtn = new QPushButton(tr("Install Xray"), corePage);
    auto* installSingBoxBtn = new QPushButton(tr("Install sing-box (experimental TUN)"),
                                              corePage);
    auto* chooseXrayBtn = new QPushButton(tr("Choose existing Xray binary"), corePage);
    auto* chooseSingBoxBtn =
        new QPushButton(tr("Choose existing sing-box binary"), corePage);
    auto* openCoreMgrBtn = new QPushButton(tr("Open Core Manager"), corePage);
    connect(installXrayBtn, &QPushButton::clicked, this,
            &FirstRunWizard::installXrayRequested);
    connect(installSingBoxBtn, &QPushButton::clicked, this,
            &FirstRunWizard::installSingBoxRequested);
    connect(chooseXrayBtn, &QPushButton::clicked, this,
            &FirstRunWizard::chooseXrayBinaryRequested);
    connect(chooseSingBoxBtn, &QPushButton::clicked, this,
            &FirstRunWizard::chooseSingBoxBinaryRequested);
    connect(openCoreMgrBtn, &QPushButton::clicked, this,
            &FirstRunWizard::openCoreManagerRequested);
    connect(corePage, &QWizardPage::completeChanged, this, [this, corePage]() {
        Q_UNUSED(corePage);
        refreshCorePage();
    });
    auto* coreLayout = new QVBoxLayout(corePage);
    coreLayout->addWidget(coreInfo);
    coreLayout->addWidget(coreHelp);
    coreLayout->addWidget(installXrayBtn);
    coreLayout->addWidget(chooseXrayBtn);
    coreLayout->addWidget(openCoreMgrBtn);
    coreLayout->addWidget(installSingBoxBtn);
    coreLayout->addWidget(chooseSingBoxBtn);
    coreLayout->addStretch();
    setPage(kPageCore, corePage);

    // Import
    auto* importPage = new QWizardPage(this);
    importPage->setTitle(tr("Import profiles"));
    auto* importWidget = new ProfileImportWidget(importPage);
    importWidget->setObjectName(QStringLiteral("profileImportWidget"));
    auto* subUrlEdit = new QLineEdit(importPage);
    subUrlEdit->setObjectName(QStringLiteral("subscriptionUrlEdit"));
    subUrlEdit->setPlaceholderText(tr("Subscription URL"));
    auto* subNameEdit = new QLineEdit(importPage);
    subNameEdit->setObjectName(QStringLiteral("subscriptionNameEdit"));
    subNameEdit->setPlaceholderText(tr("Subscription name"));
    auto* importBackupBtn = new QPushButton(tr("Import backup…"), importPage);
    auto* addManualBtn = new QPushButton(tr("Add profile manually…"), importPage);
    connect(importBackupBtn, &QPushButton::clicked, this, &FirstRunWizard::importBackupRequested);
    connect(addManualBtn, &QPushButton::clicked, this, &FirstRunWizard::addProfileManuallyRequested);
    auto* importForm = new QFormLayout;
    importForm->addRow(tr("Subscription URL"), subUrlEdit);
    importForm->addRow(tr("Name"), subNameEdit);
    auto* importLayout = new QVBoxLayout(importPage);
    importLayout->addWidget(importWidget);
    importLayout->addLayout(importForm);
    importLayout->addWidget(importBackupBtn);
    importLayout->addWidget(addManualBtn);
    setPage(kPageImport, importPage);

    // Routing/DNS
    auto* routingDnsPage = new QWizardPage(this);
    routingDnsPage->setTitle(tr("Routing and DNS"));
    auto* bypassLanRadio = new QRadioButton(tr("Bypass LAN (recommended)"), routingDnsPage);
    bypassLanRadio->setObjectName(QStringLiteral("routingBypassLan"));
    bypassLanRadio->setChecked(true);
    auto* proxyAllRadio = new QRadioButton(tr("Proxy All"), routingDnsPage);
    proxyAllRadio->setObjectName(QStringLiteral("routingProxyAll"));
    auto* bypassRuRadio = new QRadioButton(tr("Bypass LAN + RU"), routingDnsPage);
    bypassRuRadio->setObjectName(QStringLiteral("routingBypassRu"));
    auto* routingCustomRadio = new QRadioButton(tr("Custom…"), routingDnsPage);
    routingCustomRadio->setObjectName(QStringLiteral("routingCustom"));
    auto* systemDnsRadio = new QRadioButton(tr("System DNS (recommended)"), routingDnsPage);
    systemDnsRadio->setObjectName(QStringLiteral("dnsSystem"));
    systemDnsRadio->setChecked(true);
    auto* secureDnsRadio = new QRadioButton(tr("Secure Remote DNS"), routingDnsPage);
    secureDnsRadio->setObjectName(QStringLiteral("dnsSecure"));
    auto* dnsCustomRadio = new QRadioButton(tr("Custom…"), routingDnsPage);
    dnsCustomRadio->setObjectName(QStringLiteral("dnsCustom"));
    auto* routingHelp = new QLabel(
        tr("Bypass LAN keeps local/private network traffic direct.\n"
           "System DNS keeps default behavior."),
        routingDnsPage);
    routingHelp->setWordWrap(true);
    auto* openRoutingBtn = new QPushButton(tr("Open Routing Profiles"), routingDnsPage);
    auto* openDnsBtn = new QPushButton(tr("Open DNS Profiles"), routingDnsPage);
    connect(openRoutingBtn, &QPushButton::clicked, this,
            &FirstRunWizard::openRoutingProfilesRequested);
    connect(openDnsBtn, &QPushButton::clicked, this, &FirstRunWizard::openDnsProfilesRequested);
    auto* routingGroup = new QGroupBox(tr("Routing"), routingDnsPage);
    auto* routingLayout = new QVBoxLayout(routingGroup);
    routingLayout->addWidget(bypassLanRadio);
    routingLayout->addWidget(proxyAllRadio);
    routingLayout->addWidget(bypassRuRadio);
    routingLayout->addWidget(routingCustomRadio);
    auto* dnsGroup = new QGroupBox(tr("DNS"), routingDnsPage);
    auto* dnsLayout = new QVBoxLayout(dnsGroup);
    dnsLayout->addWidget(systemDnsRadio);
    dnsLayout->addWidget(secureDnsRadio);
    dnsLayout->addWidget(dnsCustomRadio);
    auto* rdLayout = new QVBoxLayout(routingDnsPage);
    rdLayout->addWidget(routingHelp);
    rdLayout->addWidget(routingGroup);
    rdLayout->addWidget(dnsGroup);
    rdLayout->addWidget(openRoutingBtn);
    rdLayout->addWidget(openDnsBtn);
    setPage(kPageRoutingDns, routingDnsPage);

    // Runtime
    auto* runtimePage = new QWizardPage(this);
    runtimePage->setTitle(tr("Runtime mode"));
    auto* systemProxyRadio =
        new QRadioButton(tr("System proxy via Xray — recommended"), runtimePage);
    systemProxyRadio->setChecked(true);
    systemProxyRadio->setObjectName(QStringLiteral("runtimeSystemProxy"));
    auto* tunRadio =
        new QRadioButton(tr("Experimental TUN via sing-box"), runtimePage);
    tunRadio->setObjectName(QStringLiteral("runtimeTun"));
    auto* tunAcceptCheck = new QCheckBox(
        tr("I understand TUN mode is experimental"), runtimePage);
    tunAcceptCheck->setObjectName(QStringLiteral("tunAcceptCheck"));
    tunAcceptCheck->setEnabled(false);
    connect(tunRadio, &QRadioButton::toggled, tunAcceptCheck, &QCheckBox::setEnabled);
    auto* tunWarning = new QLabel(
        tr("TUN mode is experimental. It may require elevated helper permissions "
           "and can change routes/firewall behavior."),
        runtimePage);
    tunWarning->setWordWrap(true);
    tunWarning->setStyleSheet(QStringLiteral("color:#b71c1c;"));
    auto* runtimeLayout = new QVBoxLayout(runtimePage);
    runtimeLayout->addWidget(systemProxyRadio);
    runtimeLayout->addWidget(tunRadio);
    runtimeLayout->addWidget(tunWarning);
    runtimeLayout->addWidget(tunAcceptCheck);
    setPage(kPageRuntime, runtimePage);

    // Finish
    auto* finishPage = new QWizardPage(this);
    finishPage->setTitle(tr("Finish"));
    auto* checklist = new FirstRunChecklistWidget(finishPage);
    checklist->setObjectName(QStringLiteral("finishChecklist"));
    auto* startNowCheck = new QCheckBox(tr("Start selected profile now"), finishPage);
    startNowCheck->setObjectName(QStringLiteral("startProfileNowCheck"));
    startNowCheck->setChecked(false);
    auto* finishLayout = new QVBoxLayout(finishPage);
    finishLayout->addWidget(checklist);
    finishLayout->addWidget(startNowCheck);
    setPage(kPageFinish, finishPage);

    setStartId(kPageWelcome);
    setOption(QWizard::NoBackButtonOnStartPage, false);
    button(QWizard::CancelButton)->setText(tr("Skip setup"));
}

void FirstRunWizard::refreshCorePage()
{
    if (!m_coreManager) {
        return;
    }
    m_coreManager->refreshLocalState();
    const CoreInfo xray = m_coreManager->infoFor(CoreType::Xray);
    const CoreInfo singBox = m_coreManager->infoFor(CoreType::SingBox);
    auto* label = findChild<QLabel*>(QStringLiteral("coreStatusLabel"));
    if (!label) {
        return;
    }
    label->setText(
        tr("Xray: %1 (%2)\nsing-box: %3 (%4)")
            .arg(xray.exists ? tr("installed") : tr("missing"),
                 xray.installedVersion.isEmpty() ? QStringLiteral("—") : xray.installedVersion,
                 singBox.exists ? tr("installed") : tr("missing"),
                 singBox.installedVersion.isEmpty() ? QStringLiteral("—")
                                                    : singBox.installedVersion));
}

bool FirstRunWizard::validateCurrentPage()
{
    if (currentId() == kPageRuntime) {
        auto* tunRadio = findChild<QRadioButton*>(QStringLiteral("runtimeTun"));
        auto* tunAcceptCheck = findChild<QCheckBox*>(QStringLiteral("tunAcceptCheck"));
        if (tunRadio && tunRadio->isChecked() && tunAcceptCheck && !tunAcceptCheck->isChecked()) {
            return false;
        }
    }
    if (currentId() == kPageFinish) {
        refreshCorePage();
        auto* checklist = findChild<FirstRunChecklistWidget*>(QStringLiteral("finishChecklist"));
        if (checklist && m_coreManager) {
            const CoreInfo xray = m_coreManager->infoFor(CoreType::Xray);
            int profileCount = m_state.importedProfiles.size();
            checklist->updateFromState(m_state, profileCount, xray.exists, xray.installedVersion);
        }
    }
    return QWizard::validateCurrentPage();
}

void FirstRunWizard::accept()
{
    // Import page
    if (auto* importWidget = findChild<ProfileImportWidget*>(QStringLiteral("profileImportWidget"))) {
        m_state.importedProfiles = importWidget->importedProfiles();
    }
    if (auto* subUrl = findChild<QLineEdit*>(QStringLiteral("subscriptionUrlEdit"))) {
        const QString url = subUrl->text().trimmed();
        if (!url.isEmpty()) {
            Subscription sub;
            sub.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            sub.url = url;
            if (auto* subName = findChild<QLineEdit*>(QStringLiteral("subscriptionNameEdit"))) {
                sub.name = subName->text().trimmed();
            }
            if (sub.name.isEmpty()) {
                sub.name = tr("Subscription");
            }
            m_state.addedSubscriptions.append(sub);
        }
    }

    // Routing/DNS
    if (findChild<QRadioButton*>(QStringLiteral("routingProxyAll"))->isChecked()) {
        m_state.routingProfileId = RoutingBuiltinIds::proxyAll();
    } else if (findChild<QRadioButton*>(QStringLiteral("routingBypassRu"))->isChecked()) {
        m_state.routingProfileId = RoutingBuiltinIds::bypassLanAndRu();
    } else if (auto* custom = findChild<QRadioButton*>(QStringLiteral("routingCustom"));
               custom && custom->isChecked()) {
        m_state.routingProfileId = RoutingBuiltinIds::customTemplate();
    } else {
        m_state.routingProfileId = RoutingBuiltinIds::bypassLan();
    }
    if (findChild<QRadioButton*>(QStringLiteral("dnsSecure"))->isChecked()) {
        m_state.dnsProfileId = DnsBuiltinIds::secureRemote();
    } else if (auto* dnsCustom = findChild<QRadioButton*>(QStringLiteral("dnsCustom"));
               dnsCustom && dnsCustom->isChecked()) {
        m_state.dnsProfileId = DnsBuiltinIds::customTemplate();
    } else {
        m_state.dnsProfileId = DnsBuiltinIds::systemDns();
    }

    // Runtime
    if (findChild<QRadioButton*>(QStringLiteral("runtimeTun"))->isChecked()) {
        m_state.runtimeMode = RuntimeMode::TunSingBoxExperimental;
        auto* tunAcceptCheck = findChild<QCheckBox*>(QStringLiteral("tunAcceptCheck"));
        m_state.tunWarningAccepted = tunAcceptCheck && tunAcceptCheck->isChecked();
    } else {
        m_state.runtimeMode = RuntimeMode::SystemProxyXray;
    }
    if (auto* startCheck = findChild<QCheckBox*>(QStringLiteral("startProfileNowCheck"))) {
        m_state.startProfileOnFinish = startCheck->isChecked();
    }

    m_skipped = false;
    emit wizardFinishedState(m_state);
    QWizard::accept();
}

void FirstRunWizard::reject()
{
    m_skipped = true;
    QWizard::reject();
}

} // namespace zarya
