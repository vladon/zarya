#include "app/AppController.h"

#include "core/CoreManager.h"
#include "core/XrayAdapter.h"
#include "domain/ProtocolType.h"
#include "domain/RoutingMode.h"
#include "domain/RoutingProfile.h"
#include "dns/DnsManager.h"
#include "dns/DnsGeoUtils.h"
#include "dns/DnsValidator.h"
#include "dns/XrayDnsGenerator.h"
#include "domain/DnsProfileMode.h"
#include "geodata/GeoDataFileStatus.h"
#include "geodata/GeoDataManager.h"
#include "platform/SystemProxyController.h"
#include "routing/RoutingManager.h"
#include "routing/RoutingProfileValidator.h"
#include "routing/XrayRoutingGenerator.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "storage/GeoDataSettingsStore.h"
#include "runtime/RuntimeBackendFactory.h"
#include "runtime/ConfigWarning.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "helperclient/HelperProcessManager.h"
#include "rulesets/RuleSetManager.h"
#include "rulesets/RuleSetStatus.h"
#include "runtime/singbox/SingBoxTunRuntimeBackend.h"
#include "runtime/xray/XraySystemProxyRuntimeBackend.h"
#include "recovery/StartupRecovery.h"
#include "ui/SafeExitDialog.h"
#include "testing/TestManager.h"

#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

namespace zarya {

namespace {

bool vmessFailureMayBeClockSkew(const QString& text)
{
    const QString lower = text.toLower();
    return lower.contains(QStringLiteral("auth")) || lower.contains(QStringLiteral("rejected"))
           || lower.contains(QStringLiteral("invalid user"))
           || lower.contains(QStringLiteral("not found"));
}

} // namespace

AppController::AppController(CoreManager* coreManager, SystemProxyController* systemProxy,
                             XrayAdapter* xrayAdapter, TestManager* testManager,
                             RoutingManager* routingManager, GeoDataManager* geoDataManager,
                             DnsManager* dnsManager, RuleSetManager* ruleSetManager,
                             QObject* parent)
    : QObject(parent)
    , m_coreManager(coreManager)
    , m_systemProxy(systemProxy)
    , m_xrayAdapter(xrayAdapter)
    , m_testManager(testManager)
    , m_routingManager(routingManager)
    , m_geoDataManager(geoDataManager)
    , m_dnsManager(dnsManager)
    , m_ruleSetManager(ruleSetManager)
{
    m_runtimeFactory = std::make_unique<RuntimeBackendFactory>(coreManager);
    setupRuntimeBackends();

    connect(m_coreManager, &CoreManager::started, this, [this](const QString& coreName) {
        Q_UNUSED(coreName);
        m_runtimeState = RuntimeState::Running;
        emit coreStateChanged(true);
        if (m_afterCoreStarted) {
            m_afterCoreStarted();
        }
    });
    connect(m_coreManager, &CoreManager::stopped, this, [this]() {
        m_runtimeState = RuntimeState::Stopped;
        emit coreStateChanged(false);
    });
    connect(m_coreManager, &CoreManager::logLine, this, &AppController::logLine);
    connect(m_coreManager, &CoreManager::errorOccurred, this, &AppController::logLine);
}

void AppController::setDialogParent(QWidget* parent)
{
    m_dialogParent = parent;
    if (m_runtimeFactory) {
        m_runtimeFactory->singBoxTunBackend()->setDialogParent(parent);
    }
}

void AppController::setupRuntimeBackends()
{
    if (!m_runtimeFactory) {
        return;
    }

    XraySystemProxyRuntimeBackend* xrayBackend = m_runtimeFactory->xraySystemProxyBackend();
    xrayBackend->setStartHandler([this](const Profile& profile, const RuntimeStartOptions& options) {
        return startProfileSystemProxyXray(profile, options.fromAutostart);
    });
    xrayBackend->setStopHandler([this]() {
        if (!isCoreRunning()) {
            return true;
        }
        restoreSystemProxyAutomatic();
        m_coreManager->stop();
        m_activeRuntimeMode = RuntimeMode::SystemProxyXray;
        AppSettings::instance().markCleanShutdown();
        return true;
    });
    xrayBackend->setRunningHandler([this]() { return isCoreRunning(); });
    connect(xrayBackend, &IRuntimeBackend::logLine, this, &AppController::logLine);
    connect(xrayBackend, &IRuntimeBackend::errorOccurred, this, &AppController::logLine);

    SingBoxTunRuntimeBackend* singBoxBackend = m_runtimeFactory->singBoxTunBackend();
    singBoxBackend->setDialogParent(m_dialogParent);
    connect(singBoxBackend, &IRuntimeBackend::logLine, this, &AppController::logLine);
    connect(singBoxBackend, &IRuntimeBackend::errorOccurred, this, &AppController::logLine);
    connect(singBoxBackend, &IRuntimeBackend::stateChanged, this, [this](RuntimeState state) {
        m_runtimeState = state;
        if (state == RuntimeState::Running) {
            emit coreStateChanged(true);
        } else if (state == RuntimeState::Stopped || state == RuntimeState::Failed) {
            emit coreStateChanged(false);
        }
    });
}

RuntimeMode AppController::activeRuntimeMode() const
{
    return m_activeRuntimeMode;
}

HelperProcessManager* AppController::helperProcessManager() const
{
    if (!m_runtimeFactory) {
        return nullptr;
    }
    return m_runtimeFactory->singBoxTunBackend()->helperManager();
}

void AppController::setAfterCoreStartedCallback(std::function<void()> callback)
{
    m_afterCoreStarted = std::move(callback);
}

void AppController::setSaveApplicationStateCallback(std::function<bool(QString*)> callback)
{
    m_saveApplicationState = std::move(callback);
}

void AppController::setOpenGeoDataManagerCallback(std::function<void()> callback)
{
    m_openGeoDataManager = std::move(callback);
}

void AppController::setOpenDnsProfilesCallback(std::function<void()> callback)
{
    m_openDnsProfiles = std::move(callback);
}

void AppController::setOpenRuleSetManagerCallback(std::function<void()> callback)
{
    m_openRuleSetManager = std::move(callback);
}

bool AppController::confirmDnsGeoDataIfNeeded(const DnsProfile& dnsProfile)
{
    if (!m_geoDataManager || !GeoDataSettingsStore::instance().warnIfMissing()) {
        return true;
    }
    if (!DnsGeoUtils::profileUsesGeoData(dnsProfile)) {
        return true;
    }

    const QStringList references = DnsGeoUtils::geoReferencesUsed(dnsProfile);
    emit logLine(QStringLiteral("DNS profile uses geo references: %1")
                     .arg(references.join(QStringLiteral(", "))));

    if (m_geoDataManager->hasRequiredFilesForTags(references)) {
        return true;
    }

    if (!m_dialogParent) {
        return true;
    }

    const QString missingFiles = m_geoDataManager->missingFileNamesForTags(references).join(
        QStringLiteral(", "));
    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Geo data missing"));
    box.setText(QStringLiteral(
        "DNS profile uses geo rules but geo data files are missing (%1).\n\nXray validation may "
        "fail.")
                    .arg(missingFiles));
    QPushButton* openManagerButton =
        box.addButton(QStringLiteral("Open Geo Data Manager"), QMessageBox::ActionRole);
    QPushButton* continueButton = box.addButton(QStringLiteral("Continue"), QMessageBox::AcceptRole);
    QPushButton* cancelButton = box.addButton(QStringLiteral("Cancel Start"), QMessageBox::RejectRole);
    box.setDefaultButton(cancelButton);
    box.exec();

    if (box.clickedButton() == openManagerButton) {
        if (m_openGeoDataManager) {
            m_openGeoDataManager();
        }
        return false;
    }
    if (box.clickedButton() == continueButton) {
        return true;
    }
    return false;
}

bool AppController::confirmDnsWarningsIfNeeded(const DnsProfile& dnsProfile,
                                               const RoutingProfile& routingProfile)
{
    QStringList warnings = DnsValidator::warnings(dnsProfile);
    warnings.append(DnsValidator::interactionWarnings(
        dnsProfile, routingProfile.domainStrategy,
        RoutingManager::profileUsesGeoData(routingProfile)));

    if (warnings.isEmpty()) {
        return true;
    }

    for (const QString& warning : warnings) {
        emit logLine(QStringLiteral("DNS profile validation warning: %1").arg(warning));
    }

    if (!m_dialogParent) {
        return true;
    }

    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("DNS warnings"));
    box.setText(QStringLiteral("DNS profile has validation warnings:\n\n%1")
                    .arg(warnings.join(QStringLiteral("\n"))));
    QPushButton* openDnsButton =
        box.addButton(QStringLiteral("Open DNS Profiles"), QMessageBox::ActionRole);
    QPushButton* continueButton = box.addButton(QStringLiteral("Continue"), QMessageBox::AcceptRole);
    QPushButton* cancelButton = box.addButton(QStringLiteral("Cancel Start"), QMessageBox::RejectRole);
    box.setDefaultButton(cancelButton);
    box.exec();

    if (box.clickedButton() == openDnsButton) {
        if (m_openDnsProfiles) {
            m_openDnsProfiles();
        }
        return false;
    }
    if (box.clickedButton() == continueButton) {
        return true;
    }
    return false;
}

void AppController::logGeoDataContext()
{
    const QString executablePath = AppSettings::instance().resolvedXrayPath();
    emit logLine(QStringLiteral("Xray executable: %1").arg(executablePath));
    emit logLine(QStringLiteral("Xray resource directory: %1").arg(AppPaths::xrayResourceDir()));

    if (!m_geoDataManager) {
        return;
    }

    const QVector<GeoDataFileStatus> statuses = m_geoDataManager->checkAllStatus();
    for (const GeoDataFileStatus& status : statuses) {
        if (status.status == GeoDataStatus::Missing) {
            emit logLine(QStringLiteral("%1 missing").arg(status.fileName));
        } else {
            emit logLine(QStringLiteral("%1 present, size %2 bytes")
                             .arg(status.fileName)
                             .arg(status.sizeBytes));
        }
    }
}

bool AppController::confirmGeoDataIfNeeded(const RoutingProfile& routingProfile)
{
    if (!m_routingManager || !m_geoDataManager) {
        return true;
    }
    if (!GeoDataSettingsStore::instance().warnIfMissing()) {
        return true;
    }
    if (!RoutingManager::profileUsesGeoData(routingProfile)) {
        return true;
    }

    const QStringList tags = RoutingManager::geoTagsUsed(routingProfile);
    emit logLine(
        QStringLiteral("Active routing profile uses geo tags: %1").arg(tags.join(QStringLiteral(", "))));

    logGeoDataContext();

    if (m_geoDataManager->hasRequiredFilesForTags(tags)) {
        return true;
    }

    if (!m_dialogParent) {
        return true;
    }

    const QString missingFiles = m_geoDataManager->missingFileNamesForTags(tags).join(QStringLiteral(", "));
    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Geo data missing"));
    box.setText(QStringLiteral(
        "The active routing profile uses geoip/geosite rules, but geo data files are missing "
        "(%1).\n\nXray validation may fail.")
                    .arg(missingFiles));
    QPushButton* openManagerButton =
        box.addButton(QStringLiteral("Open Geo Data Manager"), QMessageBox::ActionRole);
    QPushButton* continueButton = box.addButton(QStringLiteral("Continue"), QMessageBox::AcceptRole);
    QPushButton* cancelButton = box.addButton(QStringLiteral("Cancel Start"), QMessageBox::RejectRole);
    box.setDefaultButton(cancelButton);
    box.exec();

    if (box.clickedButton() == openManagerButton) {
        if (m_openGeoDataManager) {
            m_openGeoDataManager();
        }
        return false;
    }
    if (box.clickedButton() == continueButton) {
        return true;
    }
    return false;
}

bool AppController::isCoreRunning() const
{
    if (m_coreManager && m_coreManager->isRunning()) {
        return true;
    }
    if (m_runtimeFactory && m_runtimeFactory->singBoxTunBackend()->isRunning()) {
        return true;
    }
    return false;
}

RuntimeState AppController::runtimeState() const
{
    return m_runtimeState;
}

bool AppController::recoverPreviousSession(QStringList* logLines)
{
    m_runtimeState = RuntimeState::Recovering;
    const StartupRecoveryPlan plan = StartupRecovery::detect();
    QString error;
    const bool ok = StartupRecovery::apply(plan, logLines, &error);
    m_runtimeState = RuntimeState::Stopped;
    if (!error.isEmpty() && logLines) {
        logLines->append(error);
    }
    return ok;
}

bool AppController::confirmSystemProxyChangeIfNeeded() const
{
    if (!AppSettings::instance().confirmBeforeChangingSystemProxy()) {
        return true;
    }
    if (!m_dialogParent) {
        return true;
    }
    return QMessageBox::question(
               m_dialogParent, QStringLiteral("Change system proxy"),
               QStringLiteral("Zarya will change Windows system proxy settings. Continue?"))
           == QMessageBox::Yes;
}

bool AppController::writeConfigFile(const QString& path, const QJsonObject& config,
                                    QString* error) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
    return true;
}

QString AppController::configPathFor(CoreType type) const
{
    switch (type) {
    case CoreType::Xray:
        return AppPaths::xrayConfigPath();
    case CoreType::SingBox:
        return AppPaths::singBoxConfigPath();
    }
    return {};
}

bool AppController::lastStartWasAutostart() const
{
    return m_lastStartWasAutostart;
}

bool AppController::startProfile(const Profile& profile, bool fromAutostart)
{
    m_lastStartWasAutostart = fromAutostart;
    if (!profile.enabled) {
        emit logLine(QStringLiteral("Profile is disabled."));
        return false;
    }

    if (profile.coreType != CoreType::Xray) {
        emit logLine(QStringLiteral("Only Xray profiles can be started."));
        return false;
    }

    if (m_runtimeState == RuntimeState::Starting || m_runtimeState == RuntimeState::Stopping
        || m_runtimeState == RuntimeState::Recovering) {
        emit logLine(QStringLiteral("Runtime is busy; try again shortly."));
        return false;
    }

    if (isCoreRunning()) {
        if (m_dialogParent) {
            const auto answer = QMessageBox::question(
                m_dialogParent, QStringLiteral("Profile running"),
                QStringLiteral("A profile is already running. Stop and start the selected profile?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return false;
            }
            if (!stopCurrentProfile()) {
                return false;
            }
        } else {
            emit logLine(QStringLiteral("Profile is already running."));
            return false;
        }
    }

    const AppSettings& settings = AppSettings::instance();
    if (settings.effectiveRuntimeMode() == RuntimeMode::TunSingBoxExperimental) {
        return startProfileTunSingBox(profile, fromAutostart);
    }

    return startProfileSystemProxyXray(profile, fromAutostart);
}

SingBoxConfigGenerationResult AppController::generateSingBoxTunConfig(const Profile& profile) const
{
    const AppSettings& settings = AppSettings::instance();

    RoutingProfile routingProfile = RoutingProfile::builtInProxyAll();
    if (settings.tunUseActiveRoutingProfile() && m_routingManager) {
        routingProfile = m_routingManager->activeProfile();
    }

    DnsProfile dnsProfile = DnsProfile::builtInSystemDns();
    if (settings.tunUseActiveDnsProfile() && m_dnsManager) {
        dnsProfile = m_dnsManager->activeProfile();
    }

    SingBoxConfigOptions options;
    options.enableDnsHijack =
        settings.tunEnableDnsHijack()
        && settings.tunDnsHijackMode() != TunDnsHijackMode::Disabled;
    if (m_ruleSetManager) {
        options.ruleSetContext = m_ruleSetManager->buildContext(
            routingProfile, dnsProfile, settings.tunRequireLocalRuleSets());
    }

    const SingBoxConfigGenerator generator;
    SingBoxConfigGenerationResult result =
        generator.generate(profile, routingProfile, dnsProfile, options);

    if (m_ruleSetManager) {
        for (const RequiredRuleSet& required :
             m_ruleSetManager->detectRequired(routingProfile, dnsProfile)) {
            const QString line = QStringLiteral("Rule set %1 (%2): %3")
                                   .arg(required.tag, required.sourceArea,
                                        required.available ? QStringLiteral("present")
                                                           : ruleSetStatusDisplayName(
                                                                 required.catalogStatus));
            if (!result.warnings.contains(line)) {
                result.warnings.append(line);
            }
            if (!required.available && settings.tunRequireLocalRuleSets()) {
                const QString blockingLine =
                    QStringLiteral("Required rule set %1 is missing.").arg(required.tag);
                if (!result.warnings.contains(blockingLine)) {
                    result.warnings.append(blockingLine);
                }
            }
        }
    }
    if (result.success) {
        result.classifiedWarnings = generator.classifyWarnings(result.warnings);
    }
    return result;
}

bool AppController::confirmRuleSetsIfNeeded(const RoutingProfile& routingProfile,
                                            const DnsProfile& dnsProfile)
{
    if (!m_ruleSetManager) {
        return true;
    }

    const AppSettings& settings = AppSettings::instance();
    const QVector<RequiredRuleSet> required =
        m_ruleSetManager->detectRequired(routingProfile, dnsProfile);
    bool anyMissing = false;
    for (const RequiredRuleSet& entry : required) {
        if (!entry.available) {
            anyMissing = true;
            break;
        }
    }
    if (!anyMissing) {
        return true;
    }

    QStringList lines;
    for (const RequiredRuleSet& entry : required) {
        lines.append(QStringLiteral("%1: %2")
                         .arg(entry.tag, entry.available ? QStringLiteral("present")
                                                         : QStringLiteral("missing")));
    }

    if (settings.tunRequireLocalRuleSets()) {
        if (m_dialogParent) {
            QMessageBox::critical(
                m_dialogParent, QStringLiteral("Missing sing-box rule sets"),
                QStringLiteral(
                    "The active TUN routing/DNS profiles require sing-box rule sets that are "
                    "missing:\n\n%1")
                    .arg(lines.join(QStringLiteral("\n"))));
        }
        return false;
    }

    if (!m_dialogParent) {
        return true;
    }

    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("sing-box rule sets"));
    box.setText(QStringLiteral(
        "The active TUN routing/DNS profiles reference sing-box rule sets that are missing:\n\n%1\n\n"
        "Continue anyway? sing-box check is the final authority.")
                    .arg(lines.join(QStringLiteral("\n"))));
    auto* managerButton = box.addButton(QStringLiteral("Open Rule Set Manager"),
                                        QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    auto* continueButton = box.addButton(QStringLiteral("Continue"), QMessageBox::AcceptRole);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == managerButton) {
        if (m_openRuleSetManager) {
            m_openRuleSetManager();
        }
        return false;
    }
    return box.clickedButton() == continueButton;
}

bool AppController::confirmSingBoxConfigWarningsIfNeeded(
    const SingBoxConfigGenerationResult& result)
{
    if (hasBlockingWarnings(result.classifiedWarnings)) {
        const QStringList blocking =
            warningMessages(result.classifiedWarnings, ConfigWarningSeverity::Blocking);
        emit logLine(QStringLiteral("sing-box config has blocking issues."));
        if (m_dialogParent) {
            QMessageBox::critical(
                m_dialogParent, QStringLiteral("Cannot start TUN"),
                QStringLiteral("Generated sing-box config has blocking issues:\n\n%1")
                    .arg(blocking.join(QStringLiteral("\n"))));
        }
        return false;
    }

    const QStringList warnings =
        warningMessages(result.classifiedWarnings, ConfigWarningSeverity::Warning);
    if (warnings.isEmpty()) {
        return true;
    }

    if (!m_dialogParent) {
        return true;
    }

    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("sing-box config warnings"));
    box.setText(QStringLiteral("Generated sing-box config has warnings:\n\n%1\n\nContinue?")
                    .arg(warnings.join(QStringLiteral("\n"))));
    auto* previewButton =
        box.addButton(QStringLiteral("Preview Config"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    box.addButton(QMessageBox::Yes);
    box.setDefaultButton(QMessageBox::Yes);

    while (true) {
        box.exec();
        if (box.clickedButton() == previewButton) {
            const QString json =
                QString::fromUtf8(QJsonDocument(result.config).toJson(QJsonDocument::Indented));
            QMessageBox preview(m_dialogParent);
            preview.setWindowTitle(QStringLiteral("sing-box config preview"));
            preview.setText(json);
            preview.setStandardButtons(QMessageBox::Close);
            preview.exec();
            continue;
        }
        if (box.clickedButton() == box.button(QMessageBox::Cancel)) {
            return false;
        }
        return true;
    }
}

bool AppController::startProfileTunSingBox(const Profile& profile, bool fromAutostart)
{
    const AppSettings& settings = AppSettings::instance();
    if (!settings.enableExperimentalTun()) {
        emit logLine(QStringLiteral("Experimental TUN mode is not enabled in Settings."));
        return false;
    }
    if (!m_runtimeFactory) {
        return false;
    }

    QString unsupportedReason;
    const SingBoxConfigGenerator generator;
    if (!generator.supportsProfile(profile, &unsupportedReason)) {
        emit logLine(QStringLiteral("Unsupported profile: %1").arg(unsupportedReason));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Unsupported profile"),
                                 unsupportedReason);
        }
        return false;
    }

    RoutingProfile routingProfile = RoutingProfile::builtInProxyAll();
    if (settings.tunUseActiveRoutingProfile() && m_routingManager) {
        routingProfile = m_routingManager->activeProfile();
        emit logLine(QStringLiteral("Active routing profile: %1").arg(routingProfile.name));
        emit logLine(QStringLiteral("Generating TUN route: %1")
                         .arg(routingModeDisplayString(routingProfile.mode)));

        const QStringList routingWarnings = RoutingProfileValidator::warnings(routingProfile);
        for (const QString& warning : routingWarnings) {
            emit logLine(QStringLiteral("Routing validation warning: %1").arg(warning));
        }
        if (!confirmGeoDataIfNeeded(routingProfile)) {
            return false;
        }
    }

    DnsProfile dnsProfile = DnsProfile::builtInSystemDns();
    if (settings.tunUseActiveDnsProfile() && m_dnsManager) {
        dnsProfile = m_dnsManager->activeProfile();
        emit logLine(QStringLiteral("Active DNS profile: %1").arg(dnsProfile.name));
        if (!confirmDnsGeoDataIfNeeded(dnsProfile)) {
            return false;
        }
        if (!confirmDnsWarningsIfNeeded(dnsProfile, routingProfile)) {
            return false;
        }
    }

    if (!confirmRuleSetsIfNeeded(routingProfile, dnsProfile)) {
        emit logLine(QStringLiteral("TUN start canceled due to missing rule sets."));
        return false;
    }

    const SingBoxConfigGenerationResult generation = generateSingBoxTunConfig(profile);
    if (!generation.success) {
        emit logLine(QStringLiteral("Config generation failed: %1").arg(generation.errorMessage));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Config generation"),
                                 generation.errorMessage);
        }
        return false;
    }

    for (const QString& warning : generation.warnings) {
        emit logLine(QStringLiteral("sing-box config warning: %1").arg(warning));
    }

    if (!confirmSingBoxConfigWarningsIfNeeded(generation)) {
        emit logLine(QStringLiteral("TUN start canceled due to config warnings."));
        return false;
    }

    RuntimeStartOptions options;
    options.fromAutostart = fromAutostart;
    options.useActiveRoutingProfile = settings.tunUseActiveRoutingProfile();
    options.useActiveDnsProfile = settings.tunUseActiveDnsProfile();
    options.routingProfile = routingProfile;
    options.dnsProfile = dnsProfile;
    options.configWarningsAcknowledged = true;

    const bool started = m_runtimeFactory->singBoxTunBackend()->start(profile, options);
    if (started) {
        m_activeRuntimeMode = RuntimeMode::TunSingBoxExperimental;
    }
    return started;
}

bool AppController::startProfileSystemProxyXray(const Profile& profile, bool fromAutostart)
{
    Q_UNUSED(fromAutostart);
    QString unsupportedReason;
    if (!m_xrayAdapter->supportsProfile(profile, &unsupportedReason)) {
        emit logLine(QStringLiteral("Unsupported profile: %1").arg(unsupportedReason));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Unsupported profile"),
                                 unsupportedReason);
        }
        return false;
    }

    emit logLine(QStringLiteral("Generating Xray outbound: %1")
                     .arg(protocolTypeToString(profile.protocol)));
    if (!profile.network.trimmed().isEmpty()) {
        emit logLine(QStringLiteral("Network: %1").arg(profile.network));
    }
    if (!profile.security.trimmed().isEmpty()) {
        emit logLine(QStringLiteral("Security: %1").arg(profile.security));
    }

    RoutingProfile routingProfile = RoutingProfile::builtInProxyAll();
    if (m_routingManager) {
        routingProfile = m_routingManager->activeProfile();
        emit logLine(QStringLiteral("Active routing profile: %1").arg(routingProfile.name));
        emit logLine(QStringLiteral("Generating routing config: %1")
                         .arg(routingModeDisplayString(routingProfile.mode)));

        const XrayRoutingGenerator routingGenerator;
        const int ruleCount = routingGenerator.enabledRuleCount(routingProfile);
        emit logLine(QStringLiteral("Routing rules generated: %1").arg(ruleCount));

        const QStringList warnings = RoutingProfileValidator::warnings(routingProfile);
        for (const QString& warning : warnings) {
            emit logLine(QStringLiteral("Routing validation warning: %1").arg(warning));
        }
        if (!warnings.isEmpty() && m_dialogParent) {
            const auto answer = QMessageBox::question(
                m_dialogParent, QStringLiteral("Routing warnings"),
                QStringLiteral(
                    "Routing profile has validation warnings:\n\n%1\n\nContinue?")
                    .arg(warnings.join(QStringLiteral("\n"))),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return false;
            }
        }

        if (!confirmGeoDataIfNeeded(routingProfile)) {
            return false;
        }
    }

    DnsProfile dnsProfile = DnsProfile::builtInSystemDns();
    if (m_dnsManager) {
        dnsProfile = m_dnsManager->activeProfile();
        emit logLine(QStringLiteral("Active DNS profile: %1").arg(dnsProfile.name));

        const XrayDnsGenerator dnsGenerator;
        if (dnsGenerator.shouldGenerateDnsObject(dnsProfile)) {
            emit logLine(QStringLiteral("Generating DNS config"));
            emit logLine(QStringLiteral("DNS servers generated: %1")
                             .arg(dnsGenerator.enabledServerCount(dnsProfile)));
        } else {
            emit logLine(QStringLiteral("DNS config omitted: System DNS selected"));
        }

        if (!confirmDnsGeoDataIfNeeded(dnsProfile)) {
            return false;
        }
        if (!confirmDnsWarningsIfNeeded(dnsProfile, routingProfile)) {
            return false;
        }
    }

    XrayInboundPorts ports;
    const AppSettings& settings = AppSettings::instance();
    ports.socksPort = settings.socksPort();
    ports.httpPort = settings.httpPort();

    const ConfigGenerationResult generation =
        m_xrayAdapter->generateConfig(profile, ports, routingProfile, dnsProfile);
    if (!generation.success) {
        emit logLine(QStringLiteral("Config generation failed: %1").arg(generation.errorMessage));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Config generation"),
                                 generation.errorMessage);
        }
        return false;
    }

    const QString configPath = configPathFor(profile.coreType);
    QString writeError;
    if (!writeConfigFile(configPath, generation.config, &writeError)) {
        emit logLine(QStringLiteral("Failed to write config: %1").arg(writeError));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Config write"), writeError);
        }
        return false;
    }

    emit logLine(QStringLiteral("Config path: %1").arg(configPath));

    const QString executablePath = AppSettings::instance().resolvedXrayPath();
    logGeoDataContext();
    if (!QFileInfo::exists(executablePath)) {
        const QString message =
            QStringLiteral("Xray executable not found:\n%1\n\nConfigure the path in Settings.")
                .arg(executablePath);
        emit logLine(message);
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Xray not found"), message);
        }
        return false;
    }

    emit logLine(QStringLiteral("Validating Xray config…"));
    const CoreValidationResult validation =
        m_coreManager->validateConfig(executablePath, configPath);
    if (!validation.output.isEmpty()) {
        emit logLine(validation.output);
    }
    if (!validation.success) {
        if (profile.protocol == ProtocolType::Vmess) {
            emit logLine(QStringLiteral("Validation failed for VMess profile"));
            if (vmessFailureMayBeClockSkew(validation.output + validation.errorMessage)) {
                emit logLine(
                    QStringLiteral("VMess note: check system UTC time synchronization."));
            }
        }
        emit logLine(QStringLiteral("Validation failed."));
        if (m_dialogParent) {
            QMessageBox::warning(m_dialogParent, QStringLiteral("Config validation failed"),
                                 validation.errorMessage);
        }
        return false;
    }
    emit logLine(QStringLiteral("Validation OK"));

    emit logLine(QStringLiteral("Starting Xray…"));
    m_runtimeState = RuntimeState::Starting;
    m_coreManager->startCore(executablePath, configPath, m_xrayAdapter->displayName());
    AppSettings::instance().setLastStartedProfileId(profile.id);
    m_activeRuntimeMode = RuntimeMode::SystemProxyXray;
    AppSettings::instance().markCleanShutdown();
    return true;
}

bool AppController::stopRuntime()
{
    return stopCurrentProfile();
}

bool AppController::restartRuntime(const Profile& profile)
{
    if (!stopCurrentProfile()) {
        return false;
    }
    return startProfile(profile, false);
}

bool AppController::startProfileById(const QString& profileId, bool fromAutostart)
{
    Q_UNUSED(profileId);
    Q_UNUSED(fromAutostart);
    emit logLine(QStringLiteral("startProfileById requires profile lookup in MainWindow."));
    return false;
}

bool AppController::stopCurrentProfile()
{
    if (!isCoreRunning()) {
        m_runtimeState = RuntimeState::Stopped;
        return true;
    }
    if (m_runtimeState == RuntimeState::Stopping) {
        return false;
    }
    m_runtimeState = RuntimeState::Stopping;
    emit logLine(QStringLiteral("Stopping core…"));

    if (m_activeRuntimeMode == RuntimeMode::TunSingBoxExperimental && m_runtimeFactory) {
        const bool stopped = m_runtimeFactory->singBoxTunBackend()->stop();
        m_activeRuntimeMode = RuntimeMode::SystemProxyXray;
        m_runtimeState = RuntimeState::Stopped;
        emit coreStateChanged(false);
        return stopped;
    }

    restoreSystemProxyAutomatic();
    m_coreManager->stop();
    m_activeRuntimeMode = RuntimeMode::SystemProxyXray;
    m_runtimeState = RuntimeState::Stopped;
    AppSettings::instance().markCleanShutdown();
    return true;
}

bool AppController::restoreSystemProxyAutomatic()
{
    if (!AppSettings::instance().restoreProxyOnExit()) {
        return true;
    }
    QString error;
    const auto writeLog = [this](const QString& line) { emit logLine(line); };
    const bool restored =
        m_systemProxy->restorePreviousProxy(SystemProxyRestoreMode::Automatic, writeLog, &error);
    emit proxyStateChanged();
    if (!restored && !error.isEmpty()) {
        emit logLine(QStringLiteral("System proxy restore failed: %1").arg(error));
    }
    return restored;
}

bool AppController::restoreSystemProxyManual()
{
    QString error;
    const auto writeLog = [this](const QString& line) { emit logLine(line); };
    const bool restored =
        m_systemProxy->restorePreviousProxy(SystemProxyRestoreMode::Manual, writeLog, &error);
    emit proxyStateChanged();
    if (!restored && m_dialogParent && !error.isEmpty()) {
        QMessageBox::warning(m_dialogParent, QStringLiteral("System proxy"), error);
    }
    return restored;
}

bool AppController::enableSystemProxyManual()
{
    if (!isCoreRunning()) {
        emit logLine(QStringLiteral("Core is not running."));
        return false;
    }
    if (!m_systemProxy->isSupported()) {
        emit logLine(QStringLiteral("System proxy is not supported on this platform."));
        return false;
    }
    if (!confirmSystemProxyChangeIfNeeded()) {
        return false;
    }
    QString error;
    const auto writeLog = [this](const QString& line) { emit logLine(line); };
    const bool ok = m_systemProxy->enableLocalHttpProxy(AppSettings::instance().httpPort(),
                                                        writeLog, &error);
    emit proxyStateChanged();
    if (!ok && m_dialogParent) {
        QMessageBox::warning(m_dialogParent, QStringLiteral("System proxy"), error);
    }
    return ok;
}

bool AppController::attemptProxyRestoreOnShutdown(QString* error)
{
    if (!AppSettings::instance().restoreProxyOnExit()) {
        return true;
    }
    emit logLine(QStringLiteral("Restoring system proxy"));
    const auto writeLog = [this](const QString& line) { emit logLine(line); };
    const bool restored =
        m_systemProxy->restorePreviousProxy(SystemProxyRestoreMode::Automatic, writeLog, error);
    emit proxyStateChanged();
    return restored;
}

bool AppController::safeShutdown(bool proxyExitAnyway)
{
    return safeShutdownWithOptions(proxyExitAnyway, true, true, true);
}

bool AppController::safeShutdownWithOptions(bool proxyExitAnyway, bool stopRuntime,
                                            bool restoreProxy, bool disableKillSwitch)
{
    emit logLine(QStringLiteral("Safe shutdown started"));

    if (m_testManager && m_testManager->isBusy()) {
        emit logLine(QStringLiteral("Canceling tests"));
        m_testManager->cancel();
    }

    if (stopRuntime && isCoreRunning()) {
        emit logLine(QStringLiteral("Stopping core"));
        stopCurrentProfile();
        emit coreStateChanged(false);
    }

    if (disableKillSwitch) {
        HelperProcessManager* helper = helperProcessManager();
        if (helper) {
            QString killSwitchError;
            helper->connectToHelper(&killSwitchError);
            helper->killSwitchDisable(&killSwitchError);
        }
    }

    QString proxyError;
    if (!restoreProxy || m_activeRuntimeMode == RuntimeMode::TunSingBoxExperimental) {
        proxyError.clear();
    } else if (!attemptProxyRestoreOnShutdown(&proxyError) && !proxyExitAnyway) {
        emit logLine(QStringLiteral("System proxy restore failed during shutdown"));
        return false;
    }
    if (!proxyError.isEmpty() && proxyExitAnyway) {
        emit logLine(QStringLiteral("Exit anyway: system proxy may not be restored: %1")
                         .arg(proxyError));
    }

    if (m_saveApplicationState) {
        emit logLine(QStringLiteral("Saving app state"));
        QString saveError;
        if (!m_saveApplicationState(&saveError) && !saveError.isEmpty()) {
            emit logLine(QStringLiteral("Save warning: %1").arg(saveError));
        }
    }

    AppSettings::instance().markCleanShutdown();
    emit logLine(QStringLiteral("Safe shutdown completed"));
    return true;
}

void AppController::requestQuit()
{
    emit logLine(QStringLiteral("Quit requested"));

    bool stopRuntime = true;
    bool restoreProxy = true;
    bool disableKillSwitch = true;
    if (AppSettings::instance().confirmExitWhileRunning() && isCoreRunning() && m_dialogParent) {
        SafeExitDialog dialog(m_dialogParent);
        if (dialog.exec() != QDialog::Accepted) {
            emit logLine(QStringLiteral("Safe shutdown canceled"));
            emit quitBlocked(QStringLiteral("Exit canceled by user."));
            return;
        }
        const SafeExitOptions options = dialog.options();
        stopRuntime = options.stopRuntime;
        restoreProxy = options.restoreSystemProxy;
        disableKillSwitch = options.disableKillSwitch;
    }

    if (safeShutdownWithOptions(false, stopRuntime, restoreProxy, disableKillSwitch)) {
        emit quitApproved();
        return;
    }

    if (!m_dialogParent) {
        emit quitBlocked(QStringLiteral("Proxy restore failed."));
        return;
    }

    while (true) {
        QMessageBox box(m_dialogParent);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(QStringLiteral("System proxy"));
        box.setText(QStringLiteral(
            "Zarya could not restore previous system proxy settings. Retry restore or exit "
            "anyway?"));
        QPushButton* retryButton = box.addButton(QStringLiteral("Retry"), QMessageBox::AcceptRole);
        QPushButton* exitButton =
            box.addButton(QStringLiteral("Exit Anyway"), QMessageBox::DestructiveRole);
        QPushButton* cancelButton = box.addButton(QMessageBox::Cancel);
        box.exec();

        if (box.clickedButton() == cancelButton) {
            emit logLine(QStringLiteral("Safe shutdown canceled"));
            emit quitBlocked(QStringLiteral("Exit canceled after proxy restore failure."));
            return;
        }
        if (box.clickedButton() == retryButton) {
            QString error;
            if (attemptProxyRestoreOnShutdown(&error)) {
                if (safeShutdownWithOptions(true, true, true, true)) {
                    emit quitApproved();
                    return;
                }
            } else if (m_dialogParent) {
                QMessageBox::warning(m_dialogParent, QStringLiteral("System proxy"), error);
            }
            continue;
        }
        if (box.clickedButton() == exitButton) {
            emit logLine(QStringLiteral("Exit anyway after proxy restore failure"));
            if (safeShutdownWithOptions(true, true, false, true)) {
                emit quitApproved();
            } else {
                emit quitBlocked(QStringLiteral("Shutdown failed."));
            }
            return;
        }
    }
}

} // namespace zarya
