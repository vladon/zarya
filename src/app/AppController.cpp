#include "app/AppController.h"

#include "core/CoreManager.h"
#include "core/XrayAdapter.h"
#include "domain/ProtocolType.h"
#include "platform/SystemProxyController.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include "testing/TestManager.h"

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
                             XrayAdapter* xrayAdapter, TestManager* testManager, QObject* parent)
    : QObject(parent)
    , m_coreManager(coreManager)
    , m_systemProxy(systemProxy)
    , m_xrayAdapter(xrayAdapter)
    , m_testManager(testManager)
{
    connect(m_coreManager, &CoreManager::started, this, [this](const QString& coreName) {
        Q_UNUSED(coreName);
        emit coreStateChanged(true);
        if (m_afterCoreStarted) {
            m_afterCoreStarted();
        }
    });
    connect(m_coreManager, &CoreManager::stopped, this, [this]() {
        emit coreStateChanged(false);
    });
    connect(m_coreManager, &CoreManager::logLine, this, &AppController::logLine);
    connect(m_coreManager, &CoreManager::errorOccurred, this, &AppController::logLine);
}

void AppController::setDialogParent(QWidget* parent)
{
    m_dialogParent = parent;
}

void AppController::setAfterCoreStartedCallback(std::function<void()> callback)
{
    m_afterCoreStarted = std::move(callback);
}

void AppController::setSaveApplicationStateCallback(std::function<bool(QString*)> callback)
{
    m_saveApplicationState = std::move(callback);
}

bool AppController::isCoreRunning() const
{
    return m_coreManager && m_coreManager->isRunning();
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

bool AppController::startProfile(const Profile& profile)
{
    if (!profile.enabled) {
        emit logLine(QStringLiteral("Profile is disabled."));
        return false;
    }

    if (profile.coreType != CoreType::Xray) {
        emit logLine(QStringLiteral("Only Xray profiles can be started."));
        return false;
    }

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

    const ConfigGenerationResult generation = m_xrayAdapter->generateConfig(profile);
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
    m_coreManager->startCore(executablePath, configPath, m_xrayAdapter->displayName());
    return true;
}

bool AppController::stopCurrentProfile()
{
    if (!isCoreRunning()) {
        return true;
    }
    emit logLine(QStringLiteral("Stopping core…"));
    restoreSystemProxyAutomatic();
    m_coreManager->stop();
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
    emit logLine(QStringLiteral("Safe shutdown started"));

    if (m_testManager && m_testManager->isBusy()) {
        emit logLine(QStringLiteral("Canceling tests"));
        m_testManager->cancel();
    }

    if (isCoreRunning()) {
        emit logLine(QStringLiteral("Stopping core"));
        m_coreManager->stop();
        emit coreStateChanged(false);
    }

    QString proxyError;
    if (!attemptProxyRestoreOnShutdown(&proxyError) && !proxyExitAnyway) {
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

    emit logLine(QStringLiteral("Safe shutdown completed"));
    return true;
}

void AppController::requestQuit()
{
    emit logLine(QStringLiteral("Quit requested"));

    if (AppSettings::instance().confirmExitWhileRunning() && isCoreRunning() && m_dialogParent) {
        const auto answer = QMessageBox::question(
            m_dialogParent, QStringLiteral("Exit Zarya"),
            QStringLiteral("The proxy core is still running. Exit anyway?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            emit logLine(QStringLiteral("Safe shutdown canceled"));
            emit quitBlocked(QStringLiteral("Exit canceled by user."));
            return;
        }
    }

    if (safeShutdown(false)) {
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
                if (safeShutdown(true)) {
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
            if (safeShutdown(true)) {
                emit quitApproved();
            } else {
                emit quitBlocked(QStringLiteral("Shutdown failed."));
            }
            return;
        }
    }
}

} // namespace zarya
