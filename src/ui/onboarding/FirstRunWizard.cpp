#include "ui/onboarding/FirstRunWizard.h"

#include "cores/CoreBinaryManager.h"
#include "dns/DnsManager.h"
#include "domain/DnsProfileMode.h"
#include "domain/RoutingMode.h"
#include "routing/RoutingManager.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "ui/desktopapp/ZaryaFormControls.h"
#include "ui/desktopapp/ZaryaSelector.h"
#include "ui/import/ProfileImportWidget.h"
#include "ui/onboarding/FirstRunChecklistWidget.h"

#include <QAbstractButton>
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

ZaryaActionButton* addWizardButton(
    QVBoxLayout* layout,
    const QString& text)
{
    auto* button = new ZaryaActionButton(text, layout->parentWidget());
    layout->addWidget(button);
    return button;
}

} // namespace

FirstRunWizard::FirstRunWizard(
    CoreBinaryManager* coreManager,
    RoutingManager* routingManager,
    DnsManager* dnsManager,
    QWidget* parent)
    : QWizard(parent)
    , m_coreManager(coreManager)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
{
    FirstRunState::applyDefaults(&m_state);
    setWindowTitle(tr("Zarya Setup"));
    setWizardStyle(QWizard::ModernStyle);
    resize(660, 520);
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
    auto* welcome = new QWizardPage(this);
    welcome->setTitle(tr("Welcome to Zarya"));
    auto* welcomeLayout = new QVBoxLayout(welcome);
    welcomeLayout->addWidget(new ZaryaBodyText(
        tr("Zarya is a cross-platform proxy client.\n\n"
           "Recommended setup:\n"
           "1. Confirm Xray core (bundled in release builds, or install via Core Manager)\n"
           "2. Add a profile or subscription\n"
           "3. Choose routing/DNS behavior\n"
           "4. Start a profile"),
        welcome));
    welcomeLayout->addStretch();
    setPage(kPageWelcome, welcome);

    auto* corePage = new QWizardPage(this);
    corePage->setTitle(tr("Core setup"));
    auto* coreLayout = new QVBoxLayout(corePage);
    m_coreStatus = new ZaryaBodyText({}, corePage);
    coreLayout->addWidget(m_coreStatus);
    coreLayout->addWidget(new ZaryaBodyText(
        tr("Xray is required for the default system-proxy mode.\n"
           "sing-box is only needed for experimental TUN mode."),
        corePage));
    m_installXray = addWizardButton(coreLayout, tr("Install Xray"));
    auto* chooseXray = addWizardButton(coreLayout, tr("Choose existing Xray binary"));
    auto* openCore = addWizardButton(coreLayout, tr("Open Core Manager"));
    auto* installSingBox = addWizardButton(coreLayout, tr("Install sing-box (experimental TUN)"));
    auto* chooseSingBox = addWizardButton(coreLayout, tr("Choose existing sing-box binary"));
    connect(m_installXray, &ZaryaActionButton::clicked, this, &FirstRunWizard::installXrayRequested);
    connect(chooseXray, &ZaryaActionButton::clicked, this, &FirstRunWizard::chooseXrayBinaryRequested);
    connect(openCore, &ZaryaActionButton::clicked, this, &FirstRunWizard::openCoreManagerRequested);
    connect(installSingBox, &ZaryaActionButton::clicked, this, &FirstRunWizard::installSingBoxRequested);
    connect(chooseSingBox, &ZaryaActionButton::clicked, this, &FirstRunWizard::chooseSingBoxBinaryRequested);
    coreLayout->addStretch();
    setPage(kPageCore, corePage);

    auto* importPage = new QWizardPage(this);
    importPage->setTitle(tr("Import profiles"));
    auto* importLayout = new QVBoxLayout(importPage);
    m_importWidget = new ProfileImportWidget(importPage);
    m_subscriptionUrl = new ZaryaTextField(tr("Subscription URL"), importPage);
    m_subscriptionName = new ZaryaTextField(tr("Subscription name"), importPage);
    importLayout->addWidget(m_importWidget);
    importLayout->addWidget(new ZaryaFormRow(tr("Subscription URL"), m_subscriptionUrl, importPage));
    importLayout->addWidget(new ZaryaFormRow(tr("Name"), m_subscriptionName, importPage));
    auto* importBackup = addWizardButton(importLayout, tr("Import backup…"));
    auto* addManual = addWizardButton(importLayout, tr("Add profile manually…"));
    connect(importBackup, &ZaryaActionButton::clicked, this, &FirstRunWizard::importBackupRequested);
    connect(addManual, &ZaryaActionButton::clicked, this, &FirstRunWizard::addProfileManuallyRequested);
    setPage(kPageImport, importPage);

    auto* routingPage = new QWizardPage(this);
    routingPage->setTitle(tr("Routing and DNS"));
    auto* routingLayout = new QVBoxLayout(routingPage);
    routingLayout->addWidget(new ZaryaBodyText(
        tr("Bypass LAN keeps local/private network traffic direct.\n"
           "System DNS keeps default behavior."),
        routingPage));
    m_routing = new ZaryaSelector(routingPage);
    m_routing->setItems({
        {QStringLiteral("bypass"), tr("Bypass LAN (recommended)")},
        {QStringLiteral("proxy"), tr("Proxy All")},
        {QStringLiteral("ru"), tr("Bypass LAN + RU")},
        {QStringLiteral("custom"), tr("Custom…")},
    });
    m_dns = new ZaryaSelector(routingPage);
    m_dns->setItems({
        {QStringLiteral("system"), tr("System DNS (recommended)")},
        {QStringLiteral("secure"), tr("Secure Remote DNS")},
        {QStringLiteral("custom"), tr("Custom…")},
    });
    routingLayout->addWidget(new ZaryaFormRow(tr("Routing"), m_routing, routingPage));
    routingLayout->addWidget(new ZaryaFormRow(tr("DNS"), m_dns, routingPage));
    auto* openRouting = addWizardButton(routingLayout, tr("Open Routing Profiles"));
    auto* openDns = addWizardButton(routingLayout, tr("Open DNS Profiles"));
    connect(openRouting, &ZaryaActionButton::clicked, this, &FirstRunWizard::openRoutingProfilesRequested);
    connect(openDns, &ZaryaActionButton::clicked, this, &FirstRunWizard::openDnsProfilesRequested);
    routingLayout->addStretch();
    setPage(kPageRoutingDns, routingPage);

    auto* runtimePage = new QWizardPage(this);
    runtimePage->setTitle(tr("Runtime mode"));
    auto* runtimeLayout = new QVBoxLayout(runtimePage);
    m_runtime = new ZaryaSelector(runtimePage);
    m_runtime->setItems({
        {QStringLiteral("system"), tr("System proxy via Xray — recommended")},
        {QStringLiteral("tun"), tr("Experimental TUN via sing-box")},
    });
    runtimeLayout->addWidget(new ZaryaFormRow(tr("Runtime mode"), m_runtime, runtimePage));
    runtimeLayout->addWidget(new ZaryaBodyText(
        tr("TUN mode is experimental. It may require elevated helper permissions and can "
           "change routes/firewall behavior."),
        runtimePage));
    auto* configureHelper = addWizardButton(runtimeLayout, tr("Configure Helper"));
    auto* withoutHelper = addWizardButton(runtimeLayout, tr("Continue without helper"));
    m_tunAccepted = new ZaryaCheckBox(
        tr("I understand TUN mode is experimental"), runtimePage);
    m_tunAccepted->setEnabled(false);
    runtimeLayout->addWidget(m_tunAccepted);
    connect(configureHelper, &ZaryaActionButton::clicked, this, &FirstRunWizard::configureHelperRequested);
    connect(withoutHelper, &ZaryaActionButton::clicked, this, [this] {
        m_runtime->setCurrentKey(QStringLiteral("system"));
        m_tunAccepted->setEnabled(false);
    });
    connect(m_runtime, &ZaryaSelector::currentKeyChanged, this, [this](const QString& key) {
        m_tunAccepted->setEnabled(key == QStringLiteral("tun"));
    });
    runtimeLayout->addStretch();
    setPage(kPageRuntime, runtimePage);

    auto* finishPage = new QWizardPage(this);
    finishPage->setTitle(tr("Finish"));
    auto* finishLayout = new QVBoxLayout(finishPage);
    m_checklist = new FirstRunChecklistWidget(finishPage);
    m_startNow = new ZaryaCheckBox(tr("Start selected profile now"), finishPage);
    finishLayout->addWidget(m_checklist);
    finishLayout->addWidget(m_startNow);
    finishLayout->addStretch();
    setPage(kPageFinish, finishPage);

    setStartId(kPageWelcome);
    setOption(QWizard::NoBackButtonOnStartPage, false);
    button(QWizard::CancelButton)->setText(tr("Skip setup"));
    refreshCorePage();
}

void FirstRunWizard::refreshCorePage()
{
    if (!m_coreManager) {
        return;
    }
    m_coreManager->refreshLocalState();
    const CoreInfo xray = m_coreManager->infoFor(CoreType::Xray);
    const CoreInfo singBox = m_coreManager->infoFor(CoreType::SingBox);
    m_coreStatus->setText(
        tr("Xray: %1 (%2)\nsing-box: %3 (%4)")
            .arg(
                xray.exists ? tr("installed") : tr("missing"),
                xray.installedVersion.isEmpty() ? QStringLiteral("—") : xray.installedVersion,
                singBox.exists ? tr("installed") : tr("missing"),
                singBox.installedVersion.isEmpty() ? QStringLiteral("—") : singBox.installedVersion));
    m_installXray->setText(xray.exists ? tr("Update Xray…") : tr("Install Xray"));
}

bool FirstRunWizard::validateCurrentPage()
{
    if (currentId() == kPageRuntime
        && m_runtime->currentKey() == QStringLiteral("tun")
        && !m_tunAccepted->isChecked()) {
        UiMessagePresenter::warning(
            this,
            tr("Experimental mode"),
            tr("Confirm that you understand TUN mode is experimental."));
        return false;
    }
    if (currentId() == kPageFinish) {
        m_importWidget->parseLinks();
        m_state.importedProfiles = m_importWidget->importedProfiles();
        refreshCorePage();
        if (m_coreManager) {
            const CoreInfo xray = m_coreManager->infoFor(CoreType::Xray);
            m_checklist->updateFromState(
                m_state,
                m_state.importedProfiles.size(),
                xray.exists,
                xray.installedVersion);
            if (m_startNow->isChecked() && !xray.exists) {
                UiMessagePresenter::warning(
                    this,
                    tr("Core required"),
                    tr("Install or choose an Xray binary before starting a profile."));
                return false;
            }
        }
        if (m_startNow->isChecked() && m_state.importedProfiles.isEmpty()) {
            UiMessagePresenter::warning(
                this,
                tr("Profile required"),
                tr("Import or add at least one profile before starting."));
            return false;
        }
    }
    return QWizard::validateCurrentPage();
}

void FirstRunWizard::accept()
{
    m_importWidget->parseLinks();
    m_state.importedProfiles = m_importWidget->importedProfiles();
    m_state.addedSubscriptions.clear();
    const QString url = m_subscriptionUrl->text().trimmed();
    if (!url.isEmpty()) {
        Subscription sub;
        sub.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        sub.url = url;
        sub.name = m_subscriptionName->text().trimmed();
        if (sub.name.isEmpty()) {
            sub.name = tr("Subscription");
        }
        m_state.addedSubscriptions.append(sub);
    }

    const QString routing = m_routing->currentKey();
    m_state.routingProfileId = routing == QStringLiteral("proxy")
        ? RoutingBuiltinIds::proxyAll()
        : routing == QStringLiteral("ru")
        ? RoutingBuiltinIds::bypassLanAndRu()
        : routing == QStringLiteral("custom")
        ? RoutingBuiltinIds::customTemplate()
        : RoutingBuiltinIds::bypassLan();
    const QString dns = m_dns->currentKey();
    m_state.dnsProfileId = dns == QStringLiteral("secure")
        ? DnsBuiltinIds::secureRemote()
        : dns == QStringLiteral("custom")
        ? DnsBuiltinIds::customTemplate()
        : DnsBuiltinIds::systemDns();
    m_state.runtimeMode = m_runtime->currentKey() == QStringLiteral("tun")
        ? RuntimeMode::TunSingBoxExperimental
        : RuntimeMode::SystemProxyXray;
    m_state.tunWarningAccepted = m_state.runtimeMode == RuntimeMode::TunSingBoxExperimental
        && m_tunAccepted->isChecked();
    m_state.startProfileOnFinish = m_startNow->isChecked();

    m_skipped = false;
    Q_EMIT wizardFinishedState(m_state);
    QWizard::accept();
}

void FirstRunWizard::reject()
{
    m_skipped = true;
    QWizard::reject();
}

} // namespace zarya
