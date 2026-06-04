#include "runtime/singbox/SingBoxTunRuntimeBackend.h"

#include "core/CoreManager.h"
#include "runtime/ConfigWarning.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "runtime/singbox/SingBoxTunSupportChecker.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMessageBox>
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

void SingBoxTunRuntimeBackend::setDialogParent(QWidget* parent)
{
    m_dialogParent = parent;
}

SingBoxTunRuntimeBackend::SingBoxTunRuntimeBackend(CoreManager* coreManager, QObject* parent)
    : IRuntimeBackend(parent)
    , m_coreManager(coreManager)
{
    if (m_coreManager) {
        connect(m_coreManager, &CoreManager::started, this, [this](const QString& name) {
            Q_UNUSED(name);
            m_state = RuntimeState::Running;
            emit stateChanged(m_state);
        });
        connect(m_coreManager, &CoreManager::stopped, this, [this]() {
            m_state = RuntimeState::Stopped;
            emit stateChanged(m_state);
        });
        connect(m_coreManager, &CoreManager::logLine, this, &IRuntimeBackend::logLine);
        connect(m_coreManager, &CoreManager::errorOccurred, this,
                &IRuntimeBackend::errorOccurred);
    }
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

bool SingBoxTunRuntimeBackend::confirmPrivilegeWarnings(const RuntimeStartOptions& options)
{
    if (options.allowMissingPrivileges) {
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

bool SingBoxTunRuntimeBackend::start(const Profile& profile, const RuntimeStartOptions& options)
{
    if (!m_coreManager) {
        emit errorOccurred(QStringLiteral("Core manager is not available."));
        return false;
    }
    if (m_coreManager->isRunning()) {
        emit errorOccurred(QStringLiteral("A core process is already running."));
        return false;
    }

    emit logLine(QStringLiteral("Runtime mode: sing-box TUN experimental"));
    emit logLine(QStringLiteral("TUN mode is experimental; kill switch not implemented"));

    QString supportReason;
    if (!isSupported(&supportReason)) {
        emit errorOccurred(supportReason);
        return false;
    }

    const TunSupportResult support =
        SingBoxTunSupportChecker::check(AppSettings::instance().resolvedSingBoxPath());
    emit logLine(QStringLiteral("Checking TUN support"));
    emit logLine(QStringLiteral("Platform: %1").arg(support.platform));
    emit logLine(QStringLiteral("Privilege check: %1")
                     .arg(support.hasRequiredPrivileges
                              ? QStringLiteral("elevated/privileged")
                              : QStringLiteral("not elevated")));
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

    const QString executablePath = AppSettings::instance().resolvedSingBoxPath();
    emit logLine(QStringLiteral("sing-box executable: %1").arg(executablePath));

    RoutingProfile routingProfile = options.routingProfile;
    DnsProfile dnsProfile = options.dnsProfile;
    emit logLine(QStringLiteral("TUN routing profile: %1").arg(routingProfile.name));
    emit logLine(QStringLiteral("TUN DNS profile: %1").arg(dnsProfile.name));

    emit logLine(QStringLiteral("Generating sing-box TUN config"));
    const SingBoxConfigGenerator generator;
    const SingBoxConfigGenerationResult generation =
        generator.generate(profile, routingProfile, dnsProfile, configOptionsFromSettings());
    if (!generation.success) {
        emit errorOccurred(generation.errorMessage);
        return false;
    }

    for (const QString& warning : generation.warnings) {
        emit logLine(QStringLiteral("Config warning: %1").arg(warning));
    }

    const QString configPath = AppPaths::singBoxTunConfigPath();
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(configFile.errorString());
        return false;
    }
    configFile.write(QJsonDocument(generation.config).toJson(QJsonDocument::Indented));

    emit logLine(QStringLiteral("Validating sing-box config"));
    const CoreValidationResult validation =
        m_coreManager->validateSingBoxConfig(executablePath, configPath);
    if (!validation.output.isEmpty()) {
        emit logLine(validation.output);
    }
    if (!validation.success) {
        emit errorOccurred(validation.errorMessage);
        m_state = RuntimeState::Error;
        emit stateChanged(m_state);
        return false;
    }
    emit logLine(QStringLiteral("sing-box config validation OK"));

    m_state = RuntimeState::Starting;
    emit stateChanged(m_state);
    emit logLine(QStringLiteral("Starting sing-box TUN"));
    m_coreManager->startCore(executablePath, configPath, QStringLiteral("sing-box"));

    AppSettings::instance().markTunSessionStarted();
    AppSettings::instance().setLastStartedProfileId(profile.id);
    emit logLine(QStringLiteral("TUN mode started"));
    return true;
}

bool SingBoxTunRuntimeBackend::stop()
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
    emit logLine(
        QStringLiteral("Route cleanup is delegated to sing-box process termination."));
    emit logLine(QStringLiteral("TUN mode stopped"));
    AppSettings::instance().markCleanShutdown();
    m_state = RuntimeState::Stopped;
    emit stateChanged(m_state);
    return true;
}

bool SingBoxTunRuntimeBackend::isRunning() const
{
    return m_coreManager && m_coreManager->isRunning();
}

} // namespace zarya
