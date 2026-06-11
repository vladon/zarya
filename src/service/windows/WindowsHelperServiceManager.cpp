#include "service/windows/WindowsHelperServiceManager.h"

#include "app/BuildInfo.h"
#include "service/HelperServiceCommandLine.h"
#include "service/HelperServiceIdentity.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace zarya {

namespace {

#ifdef Q_OS_WIN
QString wideToQString(const wchar_t* text)
{
    return text ? QString::fromWCharArray(text) : QString();
}

bool queryScStatus(const QString& serviceName, DWORD* state, QString* errorMessage)
{
    QProcess process;
    process.start(QStringLiteral("sc.exe"),
                  {QStringLiteral("query"), serviceName});
    if (!process.waitForFinished(10000) || process.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(process.readAllStandardError()).trimmed();
            if (errorMessage->isEmpty()) {
                *errorMessage = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            }
        }
        return false;
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    if (output.contains(QStringLiteral("STATE"), Qt::CaseInsensitive)) {
        if (output.contains(QStringLiteral("RUNNING"), Qt::CaseInsensitive)) {
            *state = SERVICE_RUNNING;
            return true;
        }
        if (output.contains(QStringLiteral("STOPPED"), Qt::CaseInsensitive)) {
            *state = SERVICE_STOPPED;
            return true;
        }
        if (output.contains(QStringLiteral("START_PENDING"), Qt::CaseInsensitive)) {
            *state = SERVICE_START_PENDING;
            return true;
        }
        if (output.contains(QStringLiteral("STOP_PENDING"), Qt::CaseInsensitive)) {
            *state = SERVICE_STOP_PENDING;
            return true;
        }
    }
    *state = 0;
    return true;
}

bool runSc(const QStringList& args, QString* errorMessage)
{
    QProcess process;
    process.start(QStringLiteral("sc.exe"), args);
    if (!process.waitForFinished(30000)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("sc.exe timed out");
        }
        return false;
    }
    if (process.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(process.readAllStandardError()).trimmed();
            if (errorMessage->isEmpty()) {
                *errorMessage = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            }
        }
        return false;
    }
    return true;
}
#endif

} // namespace

WindowsHelperServiceManager::WindowsHelperServiceManager(QObject* parent)
    : IHelperServiceManager(parent)
{
}

QString WindowsHelperServiceManager::backendName() const
{
    return QStringLiteral("Windows Service");
}

bool WindowsHelperServiceManager::isSupported() const
{
    return true;
}

QString WindowsHelperServiceManager::buildServiceBinaryPath(const HelperServiceInstallOptions& options)
{
    return HelperServiceCommandLine::buildBinaryPath(options);
}

QString WindowsHelperServiceManager::manualInstallCommand(const HelperServiceInstallOptions& options) const
{
    const QString serviceName =
        options.serviceName.isEmpty() ? HelperServiceIdentity::internalServiceName()
                                      : options.serviceName;
    const QString displayName =
        options.displayName.isEmpty() ? HelperServiceIdentity::displayName() : options.displayName;
    const QString binPath = buildServiceBinaryPath(options);
    return QStringLiteral("sc.exe create %1 binPath= \"%2\" start= demand DisplayName= \"%3\"\n"
                          "sc.exe start %1")
        .arg(serviceName, binPath, displayName);
}

HelperServiceStatus WindowsHelperServiceManager::queryServiceStatus(const QString& serviceName) const
{
    HelperServiceStatus result;
    result.backend = backendName();
    result.serviceName = serviceName;
    result.version = BuildInfo::appVersion();

#ifdef Q_OS_WIN
    DWORD state = 0;
    QString error;
    if (!queryScStatus(serviceName, &state, &error)) {
        if (error.contains(QStringLiteral("1060"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("does not exist"), Qt::CaseInsensitive)
            || error.contains(QStringLiteral("FAILED 1060"), Qt::CaseInsensitive)) {
            result.state = HelperServiceInstallState::NotInstalled;
            return result;
        }
        result.state = HelperServiceInstallState::Failed;
        result.lastError = error;
        return result;
    }

    switch (state) {
    case SERVICE_RUNNING:
        result.state = HelperServiceInstallState::Running;
        result.privileged = true;
        break;
    case SERVICE_STOPPED:
        result.state = HelperServiceInstallState::Stopped;
        result.privileged = true;
        break;
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
        result.state = HelperServiceInstallState::Installed;
        result.privileged = true;
        break;
    default:
        result.state = HelperServiceInstallState::Installed;
        result.privileged = true;
        break;
    }
#else
    Q_UNUSED(serviceName);
    result.state = HelperServiceInstallState::Unsupported;
#endif
    return result;
}

HelperServiceStatus WindowsHelperServiceManager::status()
{
    return queryServiceStatus(HelperServiceIdentity::internalServiceName());
}

bool WindowsHelperServiceManager::isProcessElevated()
{
#ifdef Q_OS_WIN
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(elevation);
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
        elevated = elevation.TokenIsElevated;
    }
    CloseHandle(token);
    return elevated == TRUE;
#else
    return false;
#endif
}

bool WindowsHelperServiceManager::runElevatedCommand(const QString& command, QString* errorMessage)
{
#ifdef Q_OS_WIN
    QString escaped = command;
    escaped.replace(QLatin1Char('"'), QStringLiteral("`\""));
    const QString ps = QStringLiteral("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"%1\"")
                           .arg(escaped);
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = L"powershell.exe";
    info.lpParameters = reinterpret_cast<LPCWSTR>(ps.utf16());
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Administrator privileges are required (elevation declined or failed).");
        }
        return false;
    }
    if (info.hProcess) {
        WaitForSingleObject(info.hProcess, 120000);
        CloseHandle(info.hProcess);
    }
    return true;
#else
    Q_UNUSED(command);
    if (errorMessage) {
        *errorMessage = QStringLiteral("Elevation is only supported on Windows.");
    }
    return false;
#endif
}

bool WindowsHelperServiceManager::install(const HelperServiceInstallOptions& options,
                                          QString* errorMessage)
{
    const QString serviceName =
        options.serviceName.isEmpty() ? HelperServiceIdentity::internalServiceName()
                                      : options.serviceName;
    if (!QFileInfo::exists(options.helperExecutablePath)) {
        if (errorMessage) {
            *errorMessage =
                QStringLiteral("zarya-helper not found: %1").arg(options.helperExecutablePath);
        }
        return false;
    }

    const QString displayName =
        options.displayName.isEmpty() ? HelperServiceIdentity::displayName() : options.displayName;
    const QString binPath = buildServiceBinaryPath(options);

    auto doInstall = [&]() -> bool {
        QStringList createArgs = {QStringLiteral("create"),
                                  serviceName,
                                  QStringLiteral("binPath="),
                                  binPath,
                                  QStringLiteral("start="),
                                  options.manualStart ? QStringLiteral("demand")
                                                      : QStringLiteral("auto"),
                                  QStringLiteral("DisplayName="),
                                  displayName};
        if (!runSc(createArgs, errorMessage)) {
            return false;
        }
        if (options.startAfterInstall) {
            return runSc({QStringLiteral("start"), serviceName}, errorMessage);
        }
        return true;
    };

    if (isProcessElevated()) {
        const bool ok = doInstall();
        if (ok) {
            emit statusChanged();
        }
        return ok;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Installing Zarya Helper service requires Administrator privileges.\n\nManual command:\n%1")
                            .arg(manualInstallCommand(options));
    }
    return false;
}

bool WindowsHelperServiceManager::uninstall(bool recoverKillSwitch, QString* errorMessage)
{
    const QString serviceName = HelperServiceIdentity::internalServiceName();
    Q_UNUSED(recoverKillSwitch);

    auto doUninstall = [&]() -> bool {
        HelperServiceStatus current = queryServiceStatus(serviceName);
        if (current.state == HelperServiceInstallState::Running
            || current.state == HelperServiceInstallState::Installed) {
            if (!runSc({QStringLiteral("stop"), serviceName}, errorMessage)) {
                return false;
            }
        }
        if (!runSc({QStringLiteral("delete"), serviceName}, errorMessage)) {
            return false;
        }
        return true;
    };

    if (isProcessElevated()) {
        const bool ok = doUninstall();
        if (ok) {
            emit statusChanged();
        }
        return ok;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "Uninstalling Zarya Helper service requires Administrator privileges.\n\nManual recovery:\n"
            "sc.exe stop %1\n"
            "sc.exe delete %1")
                            .arg(serviceName);
    }
    return false;
}

bool WindowsHelperServiceManager::start(QString* errorMessage)
{
    const QString serviceName = HelperServiceIdentity::internalServiceName();
    const bool ok = isProcessElevated()
                        ? runSc({QStringLiteral("start"), serviceName}, errorMessage)
                        : runElevatedCommand(QStringLiteral("sc.exe start %1").arg(serviceName),
                                             errorMessage);
    if (ok) {
        emit statusChanged();
    }
    return ok;
}

bool WindowsHelperServiceManager::stop(QString* errorMessage)
{
    const QString serviceName = HelperServiceIdentity::internalServiceName();
    const bool ok = isProcessElevated()
                        ? runSc({QStringLiteral("stop"), serviceName}, errorMessage)
                        : runElevatedCommand(QStringLiteral("sc.exe stop %1").arg(serviceName),
                                             errorMessage);
    if (ok) {
        emit statusChanged();
    }
    return ok;
}

bool WindowsHelperServiceManager::restart(QString* errorMessage)
{
    if (!stop(errorMessage)) {
        return false;
    }
    return start(errorMessage);
}

QString WindowsHelperServiceManager::recoveryInstructions() const
{
    const QString serviceName = HelperServiceIdentity::internalServiceName();
    return QStringLiteral(
               "Manual recovery (Windows):\n"
               "  sc.exe stop %1\n"
               "  sc.exe delete %1\n\n"
               "If kill switch rules remain active, run zarya-helper --recover-killswitch from an "
               "elevated shell.")
        .arg(serviceName);
}

} // namespace zarya
