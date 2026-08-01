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

#include <QHBoxLayout>
#include <QStackedLayout>
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
    ZaryaFormSection* section,
    const QString& text)
{
    auto* button = new ZaryaActionButton(text, section);
    section->addWidget(button);
    return button;
}

struct WizardPage {
    QWidget* widget = nullptr;
    ZaryaFormSection* section = nullptr;
};

WizardPage addWizardPage(QStackedLayout* pages, const QString& title, QWidget* parent)
{
    auto* widget = new QWidget(parent);
    widget->setAccessibleName(title);
    auto* section = new ZaryaFormSection(title, widget);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(section);
    layout->addStretch();
    pages->addWidget(widget);
    return {widget, section};
}

} // namespace

FirstRunWizard::FirstRunWizard(
    CoreBinaryManager* coreManager,
    RoutingManager* routingManager,
    DnsManager* dnsManager,
    QWidget* parent)
    : QDialog(parent)
    , m_coreManager(coreManager)
    , m_routingManager(routingManager)
    , m_dnsManager(dnsManager)
{
    FirstRunState::applyDefaults(&m_state);
    setWindowTitle(tr("Zarya Setup"));
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
    auto* pageHost = new QWidget(this);
    m_pages = new QStackedLayout(pageHost);
    m_pages->setContentsMargins(0, 0, 0, 0);

    const auto welcome = addWizardPage(m_pages, tr("Welcome to Zarya"), pageHost);
    welcome.section->addWidget(new ZaryaBodyText(
        tr("Zarya is a cross-platform proxy client.\n\n"
           "Recommended setup:\n"
           "1. Confirm Xray core (bundled in release builds, or install via Core Manager)\n"
           "2. Add a profile or subscription\n"
           "3. Choose routing/DNS behavior\n"
           "4. Start a profile"),
        welcome.widget));

    const auto corePage = addWizardPage(m_pages, tr("Core setup"), pageHost);
    m_coreStatus = new ZaryaBodyText({}, corePage.widget);
    corePage.section->addWidget(m_coreStatus);
    corePage.section->addWidget(new ZaryaBodyText(
        tr("Xray is required for the default system-proxy mode.\n"
           "sing-box is only needed for experimental TUN mode."),
        corePage.widget));
    m_installXray = addWizardButton(corePage.section, tr("Install Xray"));
    auto* chooseXray = addWizardButton(corePage.section, tr("Choose existing Xray binary"));
    auto* openCore = addWizardButton(corePage.section, tr("Open Core Manager"));
    auto* installSingBox = addWizardButton(corePage.section, tr("Install sing-box (experimental TUN)"));
    auto* chooseSingBox = addWizardButton(corePage.section, tr("Choose existing sing-box binary"));
    connect(m_installXray, &ZaryaActionButton::clicked, this, &FirstRunWizard::installXrayRequested);
    connect(chooseXray, &ZaryaActionButton::clicked, this, &FirstRunWizard::chooseXrayBinaryRequested);
    connect(openCore, &ZaryaActionButton::clicked, this, &FirstRunWizard::openCoreManagerRequested);
    connect(installSingBox, &ZaryaActionButton::clicked, this, &FirstRunWizard::installSingBoxRequested);
    connect(chooseSingBox, &ZaryaActionButton::clicked, this, &FirstRunWizard::chooseSingBoxBinaryRequested);

    const auto importPage = addWizardPage(m_pages, tr("Import profiles"), pageHost);
    m_importWidget = new ProfileImportWidget(importPage.widget);
    m_subscriptionUrl = new ZaryaTextField(tr("Subscription URL"), importPage.widget);
    m_subscriptionName = new ZaryaTextField(tr("Subscription name"), importPage.widget);
    importPage.section->addWidget(m_importWidget);
    importPage.section->addWidget(
        new ZaryaFormRow(tr("Subscription URL"), m_subscriptionUrl, importPage.widget));
    importPage.section->addWidget(new ZaryaFormRow(tr("Name"), m_subscriptionName, importPage.widget));
    auto* importBackup = addWizardButton(importPage.section, tr("Import backup…"));
    auto* addManual = addWizardButton(importPage.section, tr("Add profile manually…"));
    connect(importBackup, &ZaryaActionButton::clicked, this, &FirstRunWizard::importBackupRequested);
    connect(addManual, &ZaryaActionButton::clicked, this, &FirstRunWizard::addProfileManuallyRequested);

    const auto routingPage = addWizardPage(m_pages, tr("Routing and DNS"), pageHost);
    routingPage.section->addWidget(new ZaryaBodyText(
        tr("Bypass LAN keeps local/private network traffic direct.\n"
           "System DNS keeps default behavior."),
        routingPage.widget));
    m_routing = new ZaryaSelector(routingPage.widget);
    m_routing->setItems({
        {QStringLiteral("bypass"), tr("Bypass LAN (recommended)")},
        {QStringLiteral("proxy"), tr("Proxy All")},
        {QStringLiteral("ru"), tr("Bypass LAN + RU")},
        {QStringLiteral("custom"), tr("Custom…")},
    });
    m_dns = new ZaryaSelector(routingPage.widget);
    m_dns->setItems({
        {QStringLiteral("system"), tr("System DNS (recommended)")},
        {QStringLiteral("secure"), tr("Secure Remote DNS")},
        {QStringLiteral("custom"), tr("Custom…")},
    });
    routingPage.section->addWidget(new ZaryaFormRow(tr("Routing"), m_routing, routingPage.widget));
    routingPage.section->addWidget(new ZaryaFormRow(tr("DNS"), m_dns, routingPage.widget));
    auto* openRouting = addWizardButton(routingPage.section, tr("Open Routing Profiles"));
    auto* openDns = addWizardButton(routingPage.section, tr("Open DNS Profiles"));
    connect(openRouting, &ZaryaActionButton::clicked, this, &FirstRunWizard::openRoutingProfilesRequested);
    connect(openDns, &ZaryaActionButton::clicked, this, &FirstRunWizard::openDnsProfilesRequested);

    const auto runtimePage = addWizardPage(m_pages, tr("Runtime mode"), pageHost);
    m_runtime = new ZaryaSelector(runtimePage.widget);
    m_runtime->setItems({
        {QStringLiteral("system"), tr("System proxy via Xray — recommended")},
        {QStringLiteral("tun"), tr("Experimental TUN via sing-box")},
    });
    runtimePage.section->addWidget(
        new ZaryaFormRow(tr("Runtime mode"), m_runtime, runtimePage.widget));
    runtimePage.section->addWidget(new ZaryaBodyText(
        tr("TUN mode is experimental. It may require elevated helper permissions and can "
           "change routes/firewall behavior."),
        runtimePage.widget));
    auto* configureHelper = addWizardButton(runtimePage.section, tr("Configure Helper"));
    auto* withoutHelper = addWizardButton(runtimePage.section, tr("Continue without helper"));
    m_tunAccepted = new ZaryaCheckBox(
        tr("I understand TUN mode is experimental"), runtimePage.widget);
    m_tunAccepted->setEnabled(false);
    runtimePage.section->addWidget(m_tunAccepted);
    connect(configureHelper, &ZaryaActionButton::clicked, this, &FirstRunWizard::configureHelperRequested);
    connect(withoutHelper, &ZaryaActionButton::clicked, this, [this] {
        m_runtime->setCurrentKey(QStringLiteral("system"));
        m_tunAccepted->setEnabled(false);
    });
    connect(m_runtime, &ZaryaSelector::currentKeyChanged, this, [this](const QString& key) {
        m_tunAccepted->setEnabled(key == QStringLiteral("tun"));
    });

    const auto finishPage = addWizardPage(m_pages, tr("Finish"), pageHost);
    m_checklist = new FirstRunChecklistWidget(finishPage.widget);
    m_startNow = new ZaryaCheckBox(tr("Start selected profile now"), finishPage.widget);
    finishPage.section->addWidget(m_checklist);
    finishPage.section->addWidget(m_startNow);

    m_back = new ZaryaActionButton(tr("Back"), this);
    m_next = new ZaryaActionButton(tr("Next"), this);
    auto* skip = new ZaryaActionButton(tr("Skip setup"), this);
    connect(m_back, &ZaryaActionButton::clicked, this, [this] {
        showPage(m_pages->currentIndex() - 1);
    });
    connect(m_next, &ZaryaActionButton::clicked, this, [this] {
        if (!validatePage()) {
            return;
        }
        if (m_pages->currentIndex() == kPageFinish) {
            accept();
        } else {
            showPage(m_pages->currentIndex() + 1);
        }
    });
    connect(skip, &ZaryaActionButton::clicked, this, &FirstRunWizard::reject);

    auto* navigation = new QHBoxLayout;
    navigation->addWidget(m_back);
    navigation->addStretch();
    navigation->addWidget(skip);
    navigation->addWidget(m_next);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    layout->addWidget(pageHost, 1);
    layout->addLayout(navigation);

    showPage(kPageWelcome);
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

bool FirstRunWizard::validatePage()
{
    if (m_pages->currentIndex() == kPageRuntime
        && m_runtime->currentKey() == QStringLiteral("tun")
        && !m_tunAccepted->isChecked()) {
        UiMessagePresenter::warning(
            this,
            tr("Experimental mode"),
            tr("Confirm that you understand TUN mode is experimental."));
        return false;
    }
    if (m_pages->currentIndex() == kPageFinish) {
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
    return true;
}

void FirstRunWizard::showPage(int index)
{
    const int bounded = qBound(kPageWelcome, index, kPageFinish);
    m_pages->setCurrentIndex(bounded);
    m_back->setEnabled(bounded > kPageWelcome);
    m_next->setText(bounded == kPageFinish ? tr("Finish") : tr("Next"));
    m_next->setFocus(Qt::OtherFocusReason);
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
    QDialog::accept();
}

void FirstRunWizard::reject()
{
    m_skipped = true;
    QDialog::reject();
}

} // namespace zarya
