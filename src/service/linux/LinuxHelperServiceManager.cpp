#include "service/linux/LinuxHelperServiceManager.h"

#include "app/BuildInfo.h"
#include "service/HelperServiceIdentity.h"

#include <QFile>
#include <QProcess>

namespace zarya {

LinuxHelperServiceManager::LinuxHelperServiceManager(QObject* parent)
    : IHelperServiceManager(parent)
{
}

QString LinuxHelperServiceManager::backendName() const
{
    return QStringLiteral("Linux systemd/polkit skeleton");
}

bool LinuxHelperServiceManager::systemdAvailable() const
{
    return QFile::exists(QStringLiteral("/run/systemd/system"))
        || QFile::exists(QStringLiteral("/usr/bin/systemctl"));
}

bool LinuxHelperServiceManager::isSupported() const
{
    return systemdAvailable();
}

HelperServiceStatus LinuxHelperServiceManager::querySystemdStatus() const
{
    HelperServiceStatus result;
    result.backend = backendName();
    result.serviceName = HelperServiceIdentity::linuxUnitName();
    result.version = BuildInfo::appVersion();

    if (!systemdAvailable()) {
        result.state = HelperServiceInstallState::Unsupported;
        result.warnings.append(QStringLiteral("systemd not detected."));
        return result;
    }

    QProcess process;
    process.start(QStringLiteral("systemctl"),
                  {QStringLiteral("is-active"), HelperServiceIdentity::linuxUnitName()});
    process.waitForFinished(5000);
    const QString active = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

    process.start(QStringLiteral("systemctl"),
                  {QStringLiteral("is-enabled"), HelperServiceIdentity::linuxUnitName()});
    process.waitForFinished(5000);
    const QString enabled = QString::fromUtf8(process.readAllStandardOutput()).trimmed();

    if (active == QStringLiteral("active")) {
        result.state = HelperServiceInstallState::Running;
        result.privileged = true;
    } else if (enabled == QStringLiteral("enabled") || enabled == QStringLiteral("disabled")) {
        result.state = HelperServiceInstallState::Stopped;
        result.privileged = true;
    } else {
        result.state = HelperServiceInstallState::NotInstalled;
        result.warnings.append(QStringLiteral("Manual install required. See docs/service/linux-systemd-polkit.md."));
    }
    return result;
}

HelperServiceStatus LinuxHelperServiceManager::status()
{
    return querySystemdStatus();
}

QString LinuxHelperServiceManager::manualInstallCommand(const HelperServiceInstallOptions& options) const
{
    const QString helper = options.helperExecutablePath.isEmpty()
                               ? QStringLiteral("/usr/lib/zarya/zarya-helper")
                               : options.helperExecutablePath;
    return QStringLiteral(
               "sudo install -m 0755 \"%1\" /usr/lib/zarya/zarya-helper\n"
               "sudo install -m 0644 packaging/linux/systemd/zarya-helper.service "
               "/etc/systemd/system/zarya-helper.service\n"
               "sudo systemctl daemon-reload\n"
               "sudo systemctl enable --now zarya-helper.service")
        .arg(helper);
}

bool LinuxHelperServiceManager::install(const HelperServiceInstallOptions&, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Automatic helper service installation is not enabled in 0.28.\n\n%1")
                            .arg(manualInstallCommand(HelperServiceInstallOptions{}));
    }
    return false;
}

bool LinuxHelperServiceManager::uninstall(bool, QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Automatic helper service uninstall is not enabled in 0.28.\n\n%1")
                            .arg(recoveryInstructions());
    }
    return false;
}

bool LinuxHelperServiceManager::start(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Use sudo systemctl start zarya-helper.service");
    }
    return false;
}

bool LinuxHelperServiceManager::stop(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Use sudo systemctl stop zarya-helper.service");
    }
    return false;
}

bool LinuxHelperServiceManager::restart(QString* errorMessage)
{
    if (errorMessage) {
        *errorMessage = QStringLiteral("Use sudo systemctl restart zarya-helper.service");
    }
    return false;
}

QString LinuxHelperServiceManager::recoveryInstructions() const
{
    return QStringLiteral(
        "Manual recovery (Linux):\n"
        "  sudo systemctl stop zarya-helper.service\n"
        "  sudo systemctl disable zarya-helper.service\n"
        "  sudo rm /etc/systemd/system/zarya-helper.service\n"
        "  sudo systemctl daemon-reload\n\n"
        "If kill switch rules remain active, run zarya-helper --recover-killswitch with appropriate privileges.");
}

} // namespace zarya
