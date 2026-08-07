#include "runtime/singbox/SingBoxTunRuntimeBackend.h"

#include "helperclient/HelperProcessManager.h"
#include "i18n/ZaryaTr.h"
#include "killswitch/KillSwitchMode.h"
#include "killswitch/KillSwitchPayloadBuilder.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "runtime/singbox/SingBoxTunSupportChecker.h"
#include "storage/AppSettings.h"
#include "ui/desktopapp/UiMessagePresenter.h"

#include <QJsonDocument>
#include <QWidget>

namespace zarya {
namespace {
SingBoxConfigOptions configOptionsFromSettings()
{
    const AppSettings& settings = AppSettings::instance();
    SingBoxConfigOptions options;
    options.enableDnsHijack = settings.tunEnableDnsHijack()
        && settings.tunDnsHijackMode() != TunDnsHijackMode::Disabled;
    return options;
}

bool confirmWarning(QWidget* parent, const QString& title, const QString& text,
                    const QString& acceptText)
{
    return UiMessagePresenter::choose(
               parent, title, text, UiMessageTone::Warning,
               {{QStringLiteral("continue"), acceptText, UiMessageActionRole::Primary, false, false},
                {QStringLiteral("cancel"), ZaryaTr::tr("Cancel"), UiMessageActionRole::Secondary, true, true}})
        == QStringLiteral("continue");
}
} // namespace

SingBoxTunRuntimeBackend::SingBoxTunRuntimeBackend(CoreManager* coreManager, QObject* parent)
    : IRuntimeBackend(parent)
    , m_helperManager(std::make_unique<HelperProcessManager>(this))
{
    Q_UNUSED(coreManager);
    connect(m_helperManager.get(), &HelperProcessManager::helperLogLine, this, &IRuntimeBackend::logLine);
    connect(m_helperManager.get(), &HelperProcessManager::tunExitedWithKillSwitchActive, this, [this] {
        emit logLine(QStringLiteral("TUN exited unexpectedly while kill switch is active."));
        if (m_dialogParent) {
            UiMessagePresenter::warning(m_dialogParent, ZaryaTr::tr("Kill switch"),
                ZaryaTr::tr("sing-box exited unexpectedly while kill switch is active.\n\n"
                            "Direct traffic may remain blocked. Use Settings → Kill Switch → Disable Now."));
        }
    });
}

SingBoxTunRuntimeBackend::~SingBoxTunRuntimeBackend() = default;
void SingBoxTunRuntimeBackend::setDialogParent(QWidget* parent) { m_dialogParent = parent; }
HelperProcessManager* SingBoxTunRuntimeBackend::helperManager() { return m_helperManager.get(); }
QString SingBoxTunRuntimeBackend::displayName() const { return QStringLiteral("sing-box TUN experimental"); }
RuntimeBackendType SingBoxTunRuntimeBackend::type() const { return RuntimeBackendType::SingBoxTunExperimental; }

bool SingBoxTunRuntimeBackend::isSupported(QString* reason) const
{
    const TunSupportResult result = SingBoxTunSupportChecker::check();
    if (!result.supported) {
        if (reason) *reason = result.reason;
        return false;
    }
    if (reason) *reason = {};
    return true;
}

bool SingBoxTunRuntimeBackend::validateProfile(const Profile& profile, QString* reason) const
{
    return SingBoxConfigGenerator().supportsProfile(profile, reason);
}

bool SingBoxTunRuntimeBackend::buildTunConfig(const Profile& profile, const RuntimeStartOptions& options,
                                               QByteArray* configJson, QString* errorMessage)
{
    emit logLine(QStringLiteral("TUN routing profile: %1").arg(options.routingProfile.name));
    emit logLine(QStringLiteral("TUN DNS profile: %1").arg(options.dnsProfile.name));
    const SingBoxConfigGenerationResult generation = SingBoxConfigGenerator().generate(
        profile, options.routingProfile, options.dnsProfile, configOptionsFromSettings());
    if (!generation.success) {
        if (errorMessage) *errorMessage = generation.errorMessage;
        return false;
    }
    for (const QString& warning : generation.warnings)
        emit logLine(QStringLiteral("Config warning: %1").arg(warning));
    if (configJson) *configJson = QJsonDocument(generation.config).toJson(QJsonDocument::Compact);
    return true;
}

bool SingBoxTunRuntimeBackend::start(const Profile& profile, const RuntimeStartOptions& options)
{
    if (isRunning()) { emit errorOccurred(QStringLiteral("TUN runtime is already active.")); return false; }
    if (wantsKillSwitch() && AppSettings::instance().tunPrivilegeMode() != TunPrivilegeMode::HelperExperimental) {
        emit errorOccurred(QStringLiteral("Kill switch requires zarya-helper mode in Settings."));
        return false;
    }
    QString reason;
    if (!isSupported(&reason)) { emit errorOccurred(reason); return false; }
    const TunSupportResult support = SingBoxTunSupportChecker::check();
    emit logLine(QStringLiteral("Embedded sing-box TUN runs only in zarya-helper."));
    for (const QString& warning : support.warnings) emit logLine(QStringLiteral("TUN warning: %1").arg(warning));
    if (!confirmPrivilegeWarnings(options)) return false;
    if (!validateProfile(profile, &reason)) { emit errorOccurred(reason); return false; }
    QByteArray configJson;
    if (!buildTunConfig(profile, options, &configJson, &reason)) { emit errorOccurred(reason); return false; }

    m_state = RuntimeState::Starting; emit stateChanged(m_state);
    const bool started = startViaHelper(profile, configJson);
    if (!started) { m_state = RuntimeState::Failed; emit stateChanged(m_state); return false; }
    AppSettings::instance().markTunSessionStarted();
    AppSettings::instance().setLastStartedProfileId(profile.id);
    m_state = RuntimeState::Running; emit stateChanged(m_state);
    emit logLine(QStringLiteral("Embedded sing-box TUN started via helper"));
    return true;
}

bool SingBoxTunRuntimeBackend::wantsKillSwitch() const
{
    const AppSettings& settings = AppSettings::instance();
    return settings.enableExperimentalKillSwitch()
        && settings.killSwitchMode() == KillSwitchMode::TunOnlyExperimental;
}

bool SingBoxTunRuntimeBackend::ensureHelperConnected(QString* errorMessage)
{
    if (m_helperManager->connectionState() == HelperConnectionState::Connected) return true;
    emit logLine(QStringLiteral("Connecting to zarya-helper"));
    return m_helperManager->startHelperDevMode(errorMessage);
}

bool SingBoxTunRuntimeBackend::enableKillSwitchViaHelper(const Profile& profile, QString* errorMessage)
{
    if (!wantsKillSwitch()) return true;
    const AppSettings& settings = AppSettings::instance();
    const KillSwitchPayloadResult built = KillSwitchPayloadBuilder::build(
        profile, settings.killSwitchAllowLan(), settings.killSwitchAllowLoopback(),
        settings.tunEnableDnsHijack() && settings.tunDnsHijackMode() != TunDnsHijackMode::Disabled);
    if (built.resolutionFailed) {
        if (errorMessage) *errorMessage = built.resolveWarning;
        return false;
    }
    if (!m_helperManager->killSwitchEnable(KillSwitchPayloadBuilder::toJson(built.rules), errorMessage)) return false;
    m_killSwitchSessionActive = true;
    return true;
}

bool SingBoxTunRuntimeBackend::disableKillSwitchViaHelper(QString* errorMessage)
{
    if (!m_killSwitchSessionActive) return true;
    if (!m_helperManager->killSwitchDisable(errorMessage)) return false;
    m_killSwitchSessionActive = false;
    return true;
}

bool SingBoxTunRuntimeBackend::startViaHelper(const Profile& profile, const QByteArray& configJson)
{
    QString error;
    if (!ensureHelperConnected(&error)) { emit errorOccurred(error); return false; }
    if (!enableKillSwitchViaHelper(profile, &error)) { emit errorOccurred(error); return false; }
    if (!m_helperManager->validateConfig(configJson, &error)) {
        if (wantsKillSwitch() && AppSettings::instance().killSwitchAutoDisableOnCleanStop()) disableKillSwitchViaHelper(nullptr);
        emit errorOccurred(error); return false;
    }
    if (!m_helperManager->startTun(configJson, AppSettings::instance().killSwitchAutoDisableOnCleanStop(), &error)) {
        emit errorOccurred(error); return false;
    }
    return true;
}

bool SingBoxTunRuntimeBackend::stop() { return stopViaHelper(); }

bool SingBoxTunRuntimeBackend::stopViaHelper()
{
    if (!isRunning()) { m_state = RuntimeState::Stopped; emit stateChanged(m_state); return true; }
    m_state = RuntimeState::Stopping; emit stateChanged(m_state);
    QString error;
    const bool autoDisable = AppSettings::instance().killSwitchAutoDisableOnCleanStop();
    if (m_helperManager->connectionState() != HelperConnectionState::Connected
        || !m_helperManager->stopTun(autoDisable, &error)) {
        emit errorOccurred(error.isEmpty() ? QStringLiteral("Helper unavailable for embedded sing-box stop.") : error);
        return false;
    }
    if (autoDisable) m_killSwitchSessionActive = false;
    AppSettings::instance().markCleanShutdown();
    m_state = RuntimeState::Stopped; emit stateChanged(m_state);
    emit logLine(QStringLiteral("Embedded sing-box TUN stopped"));
    return true;
}

bool SingBoxTunRuntimeBackend::isRunning() const { return m_state == RuntimeState::Running; }

bool SingBoxTunRuntimeBackend::confirmPrivilegeWarnings(const RuntimeStartOptions& options)
{
    if (options.allowMissingPrivileges) return true;
    emit logLine(QStringLiteral("TUN privilege mode: zarya-helper"));
    return true;
}
} // namespace zarya