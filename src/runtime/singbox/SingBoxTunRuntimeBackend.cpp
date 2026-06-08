#include "runtime/singbox/SingBoxTunRuntimeBackend.h"

#include "core/CoreManager.h"
#include "helperclient/HelperProcessManager.h"
#include "killswitch/KillSwitchMode.h"
#include "killswitch/KillSwitchPayloadBuilder.h"
#include "runtime/ConfigWarning.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "runtime/singbox/SingBoxTunSupportChecker.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

namespace zarya {

namespace {

SingBoxConfigOptions configOptionsFromSettings()
{
    const AppSettings& settings = AppSettings::instance();
    SingBoxConfigOptions options;
    options.enableDnsHijack =
        settings.tunEnableDnsHijack()
        && settings.tunDnsHijackMode() != TunDnsHijackMode::Disabled;
    return options;
}

} // namespace

SingBoxTunRuntimeBackend::SingBoxTunRuntimeBackend(CoreManager* coreManager, QObject* parent)
    : IRuntimeBackend(parent)
    , m_coreManager(coreManager)
    , m_helperManager(std::make_unique<HelperProcessManager>(this))
{
    connect(m_helperManager.get(), &HelperProcessManager::helperLogLine, this,
            &IRuntimeBackend::logLine);
    connect(m_helperManager.get(), &HelperProcessManager::tunExitedWithKillSwitchActive, this,
            [this]() {
                emit logLine(QStringLiteral(
                    "TUN exited unexpectedly while kill switch is active. Network may remain "
                    "blocked until you disable kill switch."));
                if (m_dialogParent) {
                    QMessageBox::warning(
                        m_dialogParent, QStringLiteral("Kill switch"),
                        QStringLiteral(
                            "sing-box exited unexpectedly while kill switch is active.\n\n"
                            "Direct traffic may remain blocked. Use Settings → Kill Switch → "
                            "Disable Now or recovery instructions."));
                }
            });

    if (m_coreManager) {
        connect(m_coreManager, &CoreManager::started, this, [this](const QString& name) {
            Q_UNUSED(name);
            if (!m_runningViaHelper) {
                m_state = RuntimeState::Running;
                emit stateChanged(m_state);
            }
        });
        connect(m_coreManager, &CoreManager::stopped, this, [this]() {
            if (!m_runningViaHelper) {
                m_state = RuntimeState::Stopped;
                emit stateChanged(m_state);
            }
        });
        connect(m_coreManager, &CoreManager::logLine, this, &IRuntimeBackend::logLine);
        connect(m_coreManager, &CoreManager::errorOccurred, this,
                &IRuntimeBackend::errorOccurred);
    }
}

SingBoxTunRuntimeBackend::~SingBoxTunRuntimeBackend() = default;

void SingBoxTunRuntimeBackend::setDialogParent(QWidget* parent)
{
    m_dialogParent = parent;
}

HelperProcessManager* SingBoxTunRuntimeBackend::helperManager()
{
    return m_helperManager.get();
}

QString SingBoxTunRuntimeBackend::displayName() const
{
    return QStringLiteral("sing-box TUN experimental");
}

RuntimeBackendType SingBoxTunRuntimeBackend::type() const
{
    return RuntimeBackendType::SingBoxTunExperimental;
}

bool SingBoxTunRuntimeBackend::isSupported(QString* reason) const
{
    const TunSupportResult result =
        SingBoxTunSupportChecker::check(AppSettings::instance().resolvedSingBoxPath());
    if (!result.supported) {
        if (reason) {
            *reason = result.reason;
        }
        return false;
    }
    if (reason) {
        *reason = {};
    }
    return true;
}

bool SingBoxTunRuntimeBackend::validateProfile(const Profile& profile, QString* reason) const
{
    const SingBoxConfigGenerator generator;
    return generator.supportsProfile(profile, reason);
}

bool SingBoxTunRuntimeBackend::writeTunConfig(const Profile& profile,
                                             const RuntimeStartOptions& options,
                                             QString* configPath, QString* errorMessage)
{
    emit logLine(QStringLiteral("TUN routing profile: %1").arg(options.routingProfile.name));
    emit logLine(QStringLiteral("TUN DNS profile: %1").arg(options.dnsProfile.name));
    emit logLine(QStringLiteral("Generating sing-box TUN config"));

    const SingBoxConfigGenerator generator;
    const SingBoxConfigGenerationResult generation = generator.generate(
        profile, options.routingProfile, options.dnsProfile, configOptionsFromSettings());
    if (!generation.success) {
        if (errorMessage) {
            *errorMessage = generation.errorMessage;
        }
        return false;
    }

    for (const QString& warning : generation.warnings) {
        emit logLine(QStringLiteral("Config warning: %1").arg(warning));
    }

    const QString path = AppPaths::singBoxTunConfigPath();
    QFile configFile(path);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = configFile.errorString();
        }
        return false;
    }
    configFile.write(QJsonDocument(generation.config).toJson(QJsonDocument::Indented));
    if (configPath) {
        *configPath = path;
    }
    return true;
}

bool SingBoxTunRuntimeBackend::start(const Profile& profile, const RuntimeStartOptions& options)
{
    if (isRunning()) {
        emit errorOccurred(QStringLiteral("TUN runtime is already active."));
        return false;
    }

    emit logLine(QStringLiteral("Runtime mode: sing-box TUN experimental"));
    if (wantsKillSwitch()) {
        emit logLine(QStringLiteral("Experimental kill switch requested"));
        if (AppSettings::instance().tunPrivilegeMode() != TunPrivilegeMode::HelperExperimental) {
            emit errorOccurred(
                QStringLiteral("Kill switch requires zarya-helper mode in Settings."));
            return false;
        }
        if (!AppSettings::instance().tunEnableDnsHijack()
            || AppSettings::instance().tunDnsHijackMode() == TunDnsHijackMode::Disabled) {
            emit logLine(QStringLiteral(
                "Kill switch warning: DNS hijack disabled may block or leak DNS depending on OS "
                "behavior."));
        }
    }

    QString supportReason;
    if (!isSupported(&supportReason)) {
        emit errorOccurred(supportReason);
        return false;
    }

    const TunSupportResult support =
        SingBoxTunSupportChecker::check(AppSettings::instance().resolvedSingBoxPath());
    emit logLine(QStringLiteral("Checking TUN support"));
    emit logLine(QStringLiteral("Platform: %1").arg(support.platform));
    for (const QString& warning : support.warnings) {
        emit logLine(QStringLiteral("TUN warning: %1").arg(warning));
    }

    if (!confirmPrivilegeWarnings(options)) {
        emit logLine(QStringLiteral("TUN start canceled by user."));
        return false;
    }

    QString profileReason;
    if (!validateProfile(profile, &profileReason)) {
        emit errorOccurred(profileReason);
        return false;
    }

    QString configPath;
    QString configError;
    if (!writeTunConfig(profile, options, &configPath, &configError)) {
        emit errorOccurred(configError);
        return false;
    }

    m_state = RuntimeState::Starting;
    emit stateChanged(m_state);

    const bool useHelper =
        AppSettings::instance().tunPrivilegeMode() == TunPrivilegeMode::HelperExperimental;
    const bool started = useHelper ? startViaHelper(profile, configPath) : startDirect(profile, configPath);
    if (started) {
        AppSettings::instance().markTunSessionStarted();
        AppSettings::instance().setLastStartedProfileId(profile.id);
        m_state = RuntimeState::Running;
        emit stateChanged(m_state);
        emit logLine(useHelper ? QStringLiteral("TUN mode started via helper")
                               : QStringLiteral("TUN mode started"));
    } else {
        m_state = RuntimeState::Failed;
        emit stateChanged(m_state);
    }
    return started;
}

bool SingBoxTunRuntimeBackend::startDirect(const Profile& profile, const QString& configPath)
{
    Q_UNUSED(profile);
    if (!m_coreManager) {
        emit errorOccurred(QStringLiteral("Core manager is not available."));
        return false;
    }
    if (m_coreManager->isRunning()) {
        emit errorOccurred(QStringLiteral("A core process is already running."));
        return false;
    }

    const QString executablePath = AppSettings::instance().resolvedSingBoxPath();
    emit logLine(QStringLiteral("sing-box executable: %1").arg(executablePath));
    emit logLine(QStringLiteral("Validating sing-box config"));
    const CoreValidationResult validation =
        m_coreManager->validateSingBoxConfig(executablePath, configPath);
    if (!validation.output.isEmpty()) {
        emit logLine(validation.output);
    }
    if (!validation.success) {
        emit errorOccurred(validation.errorMessage);
        return false;
    }
    emit logLine(QStringLiteral("sing-box config validation OK"));
    emit logLine(QStringLiteral("Starting sing-box TUN"));
    m_runningViaHelper = false;
    m_coreManager->startCore(executablePath, configPath, QStringLiteral("sing-box"));
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
    if (m_helperManager->connectionState() == HelperConnectionState::Connected) {
        return true;
    }
    emit logLine(QStringLiteral("Connecting to zarya-helper"));
    return m_helperManager->startHelperDevMode(errorMessage);
}

bool SingBoxTunRuntimeBackend::enableKillSwitchViaHelper(const Profile& profile,
                                                         QString* errorMessage)
{
    if (!wantsKillSwitch()) {
        return true;
    }

    const AppSettings& settings = AppSettings::instance();
    const bool blockDirectDns =
        settings.tunEnableDnsHijack()
        && settings.tunDnsHijackMode() != TunDnsHijackMode::Disabled;
    const KillSwitchPayloadResult built =
        KillSwitchPayloadBuilder::build(profile, settings.killSwitchAllowLan(),
                                        settings.killSwitchAllowLoopback(), blockDirectDns);
    if (built.resolutionFailed) {
        emit logLine(QStringLiteral("Kill switch: %1").arg(built.resolveWarning));
        if (!m_dialogParent) {
            if (errorMessage) {
                *errorMessage = built.resolveWarning;
            }
            return false;
        }
        QMessageBox box(m_dialogParent);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(QStringLiteral("Kill switch"));
        box.setText(QStringLiteral("%1\n\nEnable kill switch without resolved proxy IPs?")
                        .arg(built.resolveWarning));
        QPushButton* continueButton =
            box.addButton(QStringLiteral("Continue anyway"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() != continueButton) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Kill switch enable canceled.");
            }
            return false;
        }
    }

    emit logLine(QStringLiteral("Enabling kill switch via helper"));
    QString error;
    if (!m_helperManager->killSwitchEnable(KillSwitchPayloadBuilder::toJson(built.rules), &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    m_killSwitchSessionActive = true;
    emit logLine(QStringLiteral("Kill switch enabled"));
    return true;
}

bool SingBoxTunRuntimeBackend::disableKillSwitchViaHelper(QString* errorMessage)
{
    if (!m_killSwitchSessionActive) {
        return true;
    }
    emit logLine(QStringLiteral("Disabling kill switch via helper"));
    QString error;
    if (m_helperManager->connectionState() != HelperConnectionState::Connected) {
        if (m_dialogParent) {
            QMessageBox::warning(
                m_dialogParent, QStringLiteral("Kill switch"),
                QStringLiteral(
                    "Zarya could not contact helper to disable kill switch. Networking may "
                    "remain blocked.\n\n%1")
                    .arg(errorMessage ? *errorMessage : QString()));
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("Helper unavailable for kill switch disable.");
        }
        return false;
    }
    if (!m_helperManager->killSwitchDisable(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }
    m_killSwitchSessionActive = false;
    emit logLine(QStringLiteral("Kill switch disabled"));
    return true;
}

bool SingBoxTunRuntimeBackend::startViaHelper(const Profile& profile, const QString& configPath)
{
    QString error;
    if (!ensureHelperConnected(&error)) {
        emit errorOccurred(error);
        return false;
    }

    if (wantsKillSwitch()) {
        if (!enableKillSwitchViaHelper(profile, &error)) {
            emit errorOccurred(error);
            return false;
        }
    }

    const QString singBoxPath = AppSettings::instance().resolvedSingBoxPath();
    emit logLine(QStringLiteral("Sending validateConfig"));
    if (!m_helperManager->validateConfig(singBoxPath, configPath, &error)) {
        if (wantsKillSwitch() && AppSettings::instance().killSwitchAutoDisableOnCleanStop()) {
            disableKillSwitchViaHelper(nullptr);
        }
        emit errorOccurred(error);
        return false;
    }

    emit logLine(QStringLiteral("Sending startTun"));
    if (!m_helperManager->startTun(singBoxPath, configPath,
                                   AppSettings::instance().killSwitchAutoDisableOnCleanStop(),
                                   &error)) {
        emit errorOccurred(error);
        return false;
    }

    m_runningViaHelper = true;
    return true;
}

bool SingBoxTunRuntimeBackend::stop()
{
    if (m_runningViaHelper) {
        return stopViaHelper();
    }
    return stopDirect();
}

bool SingBoxTunRuntimeBackend::stopDirect()
{
    if (!m_coreManager || !m_coreManager->isRunning()) {
        m_state = RuntimeState::Stopped;
        emit stateChanged(m_state);
        return true;
    }

    emit logLine(QStringLiteral("Stopping sing-box TUN"));
    m_state = RuntimeState::Stopping;
    emit stateChanged(m_state);
    m_coreManager->stop();
    emit logLine(QStringLiteral("TUN mode stopped"));
    AppSettings::instance().markCleanShutdown();
    m_state = RuntimeState::Stopped;
    m_runningViaHelper = false;
    emit stateChanged(m_state);
    return true;
}

bool SingBoxTunRuntimeBackend::stopViaHelper()
{
    emit logLine(QStringLiteral("Stopping TUN via helper"));
    QString error;
    const bool autoDisableKillSwitch =
        AppSettings::instance().killSwitchAutoDisableOnCleanStop();
    if (m_helperManager->connectionState() == HelperConnectionState::Connected) {
        emit logLine(QStringLiteral("Sending stopTun"));
        if (!m_helperManager->stopTun(autoDisableKillSwitch, &error)) {
            if (m_dialogParent) {
                QMessageBox::warning(
                    m_dialogParent, QStringLiteral("Helper stop"),
                    QStringLiteral(
                        "Zarya could not contact helper to stop TUN.\n\n%1\n\nNetworking "
                        "may remain affected.")
                        .arg(error));
            }
            emit errorOccurred(error);
            return false;
        }
    } else if (m_dialogParent) {
        QMessageBox::warning(
            m_dialogParent, QStringLiteral("Helper unavailable"),
            QStringLiteral(
                "Zarya could not contact helper to stop TUN. Networking may remain affected."));
        return false;
    }

    m_runningViaHelper = false;
    if (autoDisableKillSwitch) {
        m_killSwitchSessionActive = false;
    } else if (m_killSwitchSessionActive && m_dialogParent) {
        QMessageBox::warning(
            m_dialogParent, QStringLiteral("Kill switch"),
            QStringLiteral(
                "Kill switch remains active after Stop. Network may stay restricted until you "
                "disable it in Settings."));
    }
    AppSettings::instance().markCleanShutdown();
    m_state = RuntimeState::Stopped;
    emit stateChanged(m_state);
    emit logLine(QStringLiteral("TUN mode stopped"));
    return true;
}

bool SingBoxTunRuntimeBackend::isRunning() const
{
    if (m_runningViaHelper) {
        return m_state == RuntimeState::Running;
    }
    return m_coreManager && m_coreManager->isRunning();
}

bool SingBoxTunRuntimeBackend::confirmPrivilegeWarnings(const RuntimeStartOptions& options)
{
    if (options.allowMissingPrivileges) {
        return true;
    }

    if (AppSettings::instance().tunPrivilegeMode() == TunPrivilegeMode::HelperExperimental) {
        emit logLine(QStringLiteral("TUN privilege mode: zarya-helper experimental"));
        return true;
    }

    const TunSupportResult support =
        SingBoxTunSupportChecker::check(AppSettings::instance().resolvedSingBoxPath());
    if (support.hasRequiredPrivileges || support.warnings.isEmpty()) {
        return true;
    }

    if (!m_dialogParent) {
        return true;
    }

    QMessageBox box(m_dialogParent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("TUN privileges"));
    box.setText(QStringLiteral(
        "Experimental TUN mode may require elevated privileges.\n\n%1\n\nContinue anyway?")
                    .arg(support.warnings.join(QStringLiteral("\n"))));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    return box.exec() == QMessageBox::Yes;
}

} // namespace zarya
