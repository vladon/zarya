#include "diagnostics/DiagnosticsCollector.h"

#include "app/AppController.h"
#include "core/CoreManager.h"
#include "core/ICoreAdapter.h"
#include "core/XrayAdapter.h"
#include "cores/CoreInstallStatus.h"
#include "cores/CoreBinaryManager.h"
#include "cores/CorePaths.h"
#include "cores/CoreVersionDetector.h"
#include "diagnostics/DiagnosticsRedactor.h"
#include "domain/DnsProfileMode.h"
#include "domain/ProtocolType.h"
#include "domain/RoutingMode.h"
#include "geodata/GeoDataFileStatus.h"
#include "killswitch/KillSwitchMode.h"
#include "runtime/singbox/SingBoxConfigGenerator.h"
#include "geodata/GeoDataManager.h"
#include "helperclient/HelperProcessManager.h"
#include "service/HelperServiceStatus.h"
#include "service/IHelperServiceManager.h"
#include "killswitch/KillSwitchState.h"
#include "logging/LogBuffer.h"
#include "migration/MigrationManager.h"
#include "app/BuildInfo.h"
#include "packaging/PackagingInfo.h"
#include "dns/DnsManager.h"
#include "platform/PlatformPrivilege.h"
#include "platform/SystemProxyController.h"
#include "routing/RoutingManager.h"
#include "rulesets/RuleSetManager.h"
#include "runtime/RuntimeBackendType.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSysInfo>
#include <QSystemTrayIcon>

namespace zarya {

namespace {

void putJson(DiagnosticsSnapshot* snapshot, const QString& relativePath, const QJsonObject& object)
{
    snapshot->jsonFiles.insert(relativePath, object);
}

void putText(DiagnosticsSnapshot* snapshot, const QString& relativePath, const QString& text)
{
    snapshot->textFiles.insert(relativePath, text);
}

void warn(DiagnosticsSnapshot* snapshot, const QString& message)
{
    snapshot->warnings.append(message);
}

void collectorError(DiagnosticsSnapshot* snapshot, const QString& category,
                    const QString& message)
{
    snapshot->collectorErrors.append(QStringLiteral("%1: %2").arg(category, message));
    warn(snapshot, QStringLiteral("%1 collector: %2").arg(category, message));
}

bool categoryEnabled(const DiagnosticsOptions& options, DiagnosticsCategory category)
{
    if (options.categories.isEmpty()) {
        return defaultDiagnosticsCategories().contains(category);
    }
    return options.categories.contains(category);
}

QJsonObject collectMigrationStatus()
{
    const MigrationResult& migration = MigrationManager::lastResult();
    QJsonObject object;
    object.insert(QStringLiteral("ok"), migration.ok);
    object.insert(QStringLiteral("migratedFiles"),
                  QJsonArray::fromStringList(migration.migratedFiles));
    object.insert(QStringLiteral("backups"), QJsonArray::fromStringList(migration.backups));
    object.insert(QStringLiteral("errors"), QJsonArray::fromStringList(migration.errors));
    object.insert(QStringLiteral("log"), QJsonArray::fromStringList(migration.logLines));
    return object;
}

QJsonObject collectAppInfo(const DiagnosticsContext& context)
{
    QJsonObject object;
    object.insert(QStringLiteral("appName"), QStringLiteral("Zarya"));
    object.insert(QStringLiteral("appVersion"), BuildInfo::appVersion());
    object.insert(QStringLiteral("buildCommit"), BuildInfo::buildCommit());
    object.insert(QStringLiteral("buildDateUtc"), BuildInfo::buildDateUtc());
    object.insert(QStringLiteral("buildChannel"), BuildInfo::buildChannel());
    object.insert(QStringLiteral("compilerInfo"), BuildInfo::compilerInfo());
    object.insert(QStringLiteral("migration"), collectMigrationStatus());
#ifdef NDEBUG
    object.insert(QStringLiteral("buildType"), QStringLiteral("Release"));
#else
    object.insert(QStringLiteral("buildType"), QStringLiteral("Debug"));
#endif
    object.insert(QStringLiteral("qtVersion"), QString::fromLatin1(QT_VERSION_STR));
    object.insert(QStringLiteral("portableMode"), AppPaths::isPortableMode());
    object.insert(QStringLiteral("runtimeMode"),
                  runtimeModeToString(AppSettings::instance().effectiveRuntimeMode()));
    if (context.appStartedAt.isValid()) {
        object.insert(QStringLiteral("startedAt"), context.appStartedAt.toString(Qt::ISODate));
        object.insert(QStringLiteral("uptimeSeconds"),
                      context.appStartedAt.secsTo(QDateTime::currentDateTimeUtc()));
    }
    return object;
}

QJsonObject collectPlatformInfo(const DiagnosticsContext& context)
{
    QJsonObject object;
#if defined(Q_OS_WIN)
    object.insert(QStringLiteral("os"), QStringLiteral("Windows"));
#elif defined(Q_OS_MACOS)
    object.insert(QStringLiteral("os"), QStringLiteral("macOS"));
#elif defined(Q_OS_LINUX)
    object.insert(QStringLiteral("os"), QStringLiteral("Linux"));
#else
    object.insert(QStringLiteral("os"), QSysInfo::productType());
#endif
    object.insert(QStringLiteral("osVersion"), QSysInfo::productVersion());
    object.insert(QStringLiteral("architecture"), QSysInfo::currentCpuArchitecture());
    object.insert(QStringLiteral("kernelType"), QSysInfo::kernelType());
    object.insert(QStringLiteral("kernelVersion"), QSysInfo::kernelVersion());

    const PrivilegeCheckResult privileges = PlatformPrivilege::currentProcessPrivileges();
    object.insert(QStringLiteral("isElevated"), privileges.elevated);
    object.insert(QStringLiteral("privilegeSummary"), privileges.summary);
    object.insert(QStringLiteral("systemTrayAvailable"), context.systemTrayAvailable);

#if defined(Q_OS_LINUX)
    object.insert(QStringLiteral("hasNft"),
                  QFile::exists(QStringLiteral("/sbin/nft"))
                      || QFile::exists(QStringLiteral("/usr/sbin/nft")));
    object.insert(QStringLiteral("hasDevNetTun"), QFile::exists(QStringLiteral("/dev/net/tun")));
#endif
    return object;
}

QJsonObject collectPaths(const DiagnosticsOptions& options, const DiagnosticsContext&)
{
    const auto pathField = [&](const QString& path) {
        return DiagnosticsRedactor::redactPath(path, options.redactionMode,
                                               options.includeMachinePaths);
    };

    QJsonObject object;
    object.insert(QStringLiteral("dataDir"), pathField(AppPaths::dataDir()));
    object.insert(QStringLiteral("runtimeDir"), pathField(AppPaths::runtimeDir()));
    object.insert(QStringLiteral("coresDir"), pathField(AppPaths::coresDir()));
    object.insert(QStringLiteral("xrayPath"), pathField(AppSettings::instance().resolvedXrayPath()));
    object.insert(QStringLiteral("singBoxPath"),
                  pathField(AppSettings::instance().resolvedSingBoxPath()));

    const QString xrayPath = AppSettings::instance().resolvedXrayPath();
    const QString singBoxPath = AppSettings::instance().resolvedSingBoxPath();
    object.insert(QStringLiteral("xrayPathExists"), QFile::exists(xrayPath));
    object.insert(QStringLiteral("singBoxPathExists"), QFile::exists(singBoxPath));
    object.insert(QStringLiteral("ruleSetDirExists"), QDir(AppPaths::singBoxRuleSetDir()).exists());

    QString writableError;
    object.insert(QStringLiteral("dataDirWritable"),
                  QFileInfo(AppPaths::dataDir()).isWritable());
    return object;
}

QJsonObject collectCoreStatus(const DiagnosticsContext& context, DiagnosticsSnapshot* snapshot)
{
    QJsonObject root;
    if (!context.coreBinaryManager) {
        collectorError(snapshot, QStringLiteral("coreStatus"),
                       QStringLiteral("Core binary manager unavailable"));
        return root;
    }

    context.coreBinaryManager->refreshLocalState();
    for (const CoreInfo& info : context.coreBinaryManager->coreInfos()) {
        QJsonObject core;
        core.insert(QStringLiteral("configuredPath"),
                    DiagnosticsRedactor::redactPath(info.executablePath,
                                                    DiagnosticsRedactionMode::Strict, false));
        core.insert(QStringLiteral("exists"), info.exists);
        core.insert(QStringLiteral("installedVersion"), info.installedVersion);
        core.insert(QStringLiteral("managedByZarya"), info.managed);
        core.insert(QStringLiteral("running"), info.running);
        core.insert(QStringLiteral("status"), coreInstallStatusToString(info.status));
        if (!info.lastError.isEmpty()) {
            core.insert(QStringLiteral("lastError"),
                        DiagnosticsRedactor::redactText(info.lastError,
                                                        DiagnosticsRedactionMode::Strict));
        }
        root.insert(info.type == CoreType::Xray ? QStringLiteral("xray") : QStringLiteral("singBox"),
                    core);
    }
    return root;
}

QJsonObject collectRuntimeStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    object.insert(QStringLiteral("runtimeMode"),
                  runtimeModeToString(AppSettings::instance().effectiveRuntimeMode()));
    const bool coreRunning = context.appController && context.appController->isCoreRunning();
    object.insert(QStringLiteral("coreRunning"), coreRunning);
    object.insert(QStringLiteral("processCoreRunning"),
                  context.coreManager && context.coreManager->isRunning());
    object.insert(QStringLiteral("systemProxyEnabledByZarya"),
                  context.systemProxy && context.systemProxy->enabledByZarya());
    if (context.hasSelectedProfile) {
        object.insert(QStringLiteral("selectedProfileProtocol"),
                      protocolTypeToString(context.selectedProfile.protocol));
    }
    return object;
}

QJsonObject collectHelperServiceStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    IHelperServiceManager* service = context.helperService;
    if (!service) {
        object.insert(QStringLiteral("backend"), QStringLiteral("unavailable"));
        object.insert(QStringLiteral("installed"), false);
        object.insert(QStringLiteral("running"), false);
        object.insert(QStringLiteral("connected"), false);
        return object;
    }

    const HelperServiceStatus status = service->status();
    object.insert(QStringLiteral("backend"), status.backend);
    object.insert(QStringLiteral("serviceName"), status.serviceName);
    object.insert(QStringLiteral("installed"),
                  status.state == HelperServiceInstallState::Installed
                      || status.state == HelperServiceInstallState::Running
                      || status.state == HelperServiceInstallState::Stopped);
    object.insert(QStringLiteral("running"), status.state == HelperServiceInstallState::Running);
  object.insert(QStringLiteral("connected"),
                  context.helper
                      && context.helper->connectionState() == HelperConnectionState::Connected);
    object.insert(QStringLiteral("privileged"), status.privileged);
    object.insert(QStringLiteral("version"), status.version);
    object.insert(QStringLiteral("state"), helperServiceInstallStateToString(status.state));
    if (!status.lastError.isEmpty()) {
        object.insert(QStringLiteral("lastError"), status.lastError);
    }
    QJsonArray warnings;
    for (const QString& warning : status.warnings) {
        warnings.append(warning);
    }
    object.insert(QStringLiteral("warnings"), warnings);
    return object;
}

QJsonObject collectHelperStatus(const DiagnosticsContext& context, DiagnosticsSnapshot* snapshot)
{
    QJsonObject object;
    HelperProcessManager* helper = context.helper;
    if (!helper) {
        object.insert(QStringLiteral("connected"), false);
        object.insert(QStringLiteral("lastError"), QStringLiteral("helper manager unavailable"));
        return object;
    }

    object.insert(QStringLiteral("helperMode"), QStringLiteral("external-privileged"));
    object.insert(QStringLiteral("connected"),
                  helper->connectionState() == HelperConnectionState::Connected);
    object.insert(QStringLiteral("connectionState"),
                  helper->statusText());
    object.insert(QStringLiteral("helperVersion"), PackagingInfo::versionString());
    object.insert(QStringLiteral("privileged"), helper->connectionState() == HelperConnectionState::Connected);

    QJsonObject statusPayload;
    QString statusError;
    if (helper->status(&statusPayload, &statusError)) {
        object.insert(QStringLiteral("tunSupported"), statusPayload.value(QStringLiteral("tunSupported")).toBool());
        object.insert(QStringLiteral("tunRunning"), statusPayload.value(QStringLiteral("tunRunning")).toBool());
    } else if (!statusError.isEmpty()) {
        object.insert(QStringLiteral("lastError"), statusError);
        if (helper->connectionState() != HelperConnectionState::Connected) {
            warn(snapshot, QStringLiteral("Helper not connected"));
        }
    }
    return object;
}

QJsonObject collectSystemProxyStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    if (!context.systemProxy) {
        object.insert(QStringLiteral("supported"), false);
        return object;
    }
    object.insert(QStringLiteral("backend"), context.systemProxy->backendName());
    object.insert(QStringLiteral("supported"), context.systemProxy->isSupported());
    object.insert(QStringLiteral("supportLevel"), context.systemProxy->supportLevel());
    object.insert(QStringLiteral("enabledByZarya"), context.systemProxy->enabledByZarya());
    object.insert(QStringLiteral("uiStatus"), context.systemProxy->uiStatusText());
    object.insert(QStringLiteral("hasSavedPreviousState"), context.systemProxy->hasSavedState());
    if (!context.systemProxy->lastError().isEmpty()) {
        object.insert(QStringLiteral("lastRestoreError"),
                      DiagnosticsRedactor::redactText(context.systemProxy->lastError(),
                                                      DiagnosticsRedactionMode::Strict));
    }
    object.insert(QStringLiteral("currentProxyServer"),
                  QStringLiteral("127.0.0.1:%1").arg(AppSettings::instance().httpPort()));
    return object;
}

QJsonObject collectKillSwitchStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    const AppSettings& settings = AppSettings::instance();
    object.insert(QStringLiteral("experimentalEnabled"), settings.enableExperimentalKillSwitch());
    object.insert(QStringLiteral("mode"), killSwitchModeToString(settings.killSwitchMode()));
    object.insert(QStringLiteral("recoveryMarkerPresent"),
                  QFile::exists(AppPaths::killSwitchMarkerPath()));

    if (context.helper) {
        const KillSwitchState state = context.helper->killSwitchState();
        object.insert(QStringLiteral("enabled"), state.status == KillSwitchStatus::Enabled);
        object.insert(QStringLiteral("status"), killSwitchStatusToString(state.status));
        object.insert(QStringLiteral("backend"), state.backend);
        object.insert(QStringLiteral("helperRequired"), true);
        if (!state.lastError.isEmpty()) {
            object.insert(QStringLiteral("lastError"),
                          DiagnosticsRedactor::redactText(state.lastError,
                                                          DiagnosticsRedactionMode::Strict));
        }
        QJsonArray rules;
        for (const QString& rule : state.activeRules) {
            rules.append(DiagnosticsRedactor::redactText(rule, DiagnosticsRedactionMode::Strict));
        }
        object.insert(QStringLiteral("activeRulesSummary"), rules);
    }
    return object;
}

QJsonObject collectRoutingStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    if (!context.routingManager) {
        return object;
    }
    const RoutingProfile profile = context.routingManager->activeProfile();
    object.insert(QStringLiteral("activeRoutingProfile"), profile.name);
    object.insert(QStringLiteral("mode"), routingModeToString(profile.mode));
    object.insert(QStringLiteral("rulesCount"), profile.rules.size());
    object.insert(QStringLiteral("usesGeoData"), RoutingManager::profileUsesGeoData(profile));
    object.insert(QStringLiteral("geoTagsUsed"), QJsonArray::fromStringList(
                                                     RoutingManager::geoTagsUsed(profile)));
    return object;
}

QJsonObject collectDnsStatus(const DiagnosticsContext& context)
{
    QJsonObject object;
    if (!context.dnsManager) {
        return object;
    }
    const DnsProfile profile = context.dnsManager->activeProfile();
    object.insert(QStringLiteral("activeDnsProfile"), profile.name);
    object.insert(QStringLiteral("mode"), dnsProfileModeToString(profile.mode));
    object.insert(QStringLiteral("serversCount"), profile.servers.size());
    object.insert(QStringLiteral("usesGeoData"), false);
    return object;
}

QJsonObject collectGeoDataStatus(const DiagnosticsOptions& options, const DiagnosticsContext& context)
{
    QJsonObject object;
    if (!context.geoDataManager) {
        return object;
    }
    object.insert(QStringLiteral("xrayResourceDir"),
                  DiagnosticsRedactor::redactPath(context.geoDataManager->targetDirectory(),
                                                  options.redactionMode, options.includeMachinePaths));

    const auto fileObject = [](const GeoDataFileStatus& status) {
        QJsonObject file;
        file.insert(QStringLiteral("exists"), status.status != GeoDataStatus::Missing);
        file.insert(QStringLiteral("sizeBytes"), status.sizeBytes);
        if (status.modifiedAt.isValid()) {
            file.insert(QStringLiteral("modifiedAt"), status.modifiedAt.toString(Qt::ISODate));
        }
        file.insert(QStringLiteral("status"), geoDataStatusDisplayString(status.status));
        return file;
    };

    object.insert(QStringLiteral("geoip"), fileObject(context.geoDataManager->checkFileStatus(GeoDataKind::GeoIp)));
    object.insert(QStringLiteral("geosite"),
                  fileObject(context.geoDataManager->checkFileStatus(GeoDataKind::GeoSite)));
    return object;
}

QJsonObject collectRuleSetStatus(const DiagnosticsOptions& options, const DiagnosticsContext& context)
{
    QJsonObject object;
    if (!context.ruleSetManager || !context.routingManager || !context.dnsManager) {
        return object;
    }
    object.insert(QStringLiteral("ruleSetDir"),
                  DiagnosticsRedactor::redactPath(context.ruleSetManager->targetDirectory(),
                                                  options.redactionMode, options.includeMachinePaths));
    object.insert(QStringLiteral("strictMode"), AppSettings::instance().tunRequireLocalRuleSets());

    const QVector<RequiredRuleSet> required = context.ruleSetManager->detectRequired(
        context.routingManager->activeProfile(), context.dnsManager->activeProfile());
    QJsonArray items;
    for (const RequiredRuleSet& item : required) {
        QJsonObject entry;
        entry.insert(QStringLiteral("tag"), item.tag);
        entry.insert(QStringLiteral("present"), item.available);
        if (!item.warning.isEmpty()) {
            entry.insert(QStringLiteral("warning"),
                        DiagnosticsRedactor::redactText(item.warning, options.redactionMode));
        }
        items.append(entry);
    }
    object.insert(QStringLiteral("requiredRuleSets"), items);
    return object;
}

QJsonObject collectValidation(const DiagnosticsOptions& options, const DiagnosticsContext& context,
                              DiagnosticsSnapshot* snapshot)
{
    QJsonObject root;
    if (!options.runConfigValidation) {
        root.insert(QStringLiteral("skipped"), true);
        root.insert(QStringLiteral("reason"), QStringLiteral("Validation disabled by user"));
        return root;
    }
    if (!context.hasSelectedProfile) {
        root.insert(QStringLiteral("skipped"), true);
        root.insert(QStringLiteral("reason"), QStringLiteral("No profile selected"));
        return root;
    }
    if (!context.coreManager || !context.xrayAdapter || !context.appController) {
        collectorError(snapshot, QStringLiteral("validation"),
                       QStringLiteral("Validation dependencies unavailable"));
        return root;
    }

    const Profile& profile = context.selectedProfile;
    const RoutingProfile routing = context.routingManager->activeProfile();
    const DnsProfile dns = context.dnsManager->activeProfile();

    QJsonObject xrayValidation;
    const ConfigGenerationResult xrayConfig =
        context.xrayAdapter->generateConfig(profile, {}, routing, dns);
    if (!xrayConfig.success) {
        xrayValidation.insert(QStringLiteral("attempted"), true);
        xrayValidation.insert(QStringLiteral("success"), false);
        xrayValidation.insert(QStringLiteral("stderrRedacted"),
                              DiagnosticsRedactor::redactText(xrayConfig.errorMessage,
                                                              options.redactionMode));
    } else {
        const QString configPath =
            QDir(AppPaths::testRuntimeDir())
                .filePath(QStringLiteral("diagnostics-xray-config.json"));
        QFile file(configPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(xrayConfig.config).toJson(QJsonDocument::Compact));
        }
        const CoreValidationResult result = context.coreManager->validateConfig(
            AppSettings::instance().resolvedXrayPath(), configPath);
        xrayValidation.insert(QStringLiteral("attempted"), true);
        xrayValidation.insert(QStringLiteral("exitCode"), result.exitCode);
        xrayValidation.insert(QStringLiteral("success"), result.success);
        xrayValidation.insert(QStringLiteral("stdoutRedacted"),
                              DiagnosticsRedactor::redactText(result.output, options.redactionMode));
        xrayValidation.insert(QStringLiteral("stderrRedacted"),
                              DiagnosticsRedactor::redactText(result.errorMessage,
                                                              options.redactionMode));
        if (!result.success) {
            warn(snapshot, QStringLiteral("Xray config validation failed"));
        }
    }
    root.insert(QStringLiteral("xrayConfigValidation"), xrayValidation);

    QJsonObject singBoxValidation;
    const SingBoxConfigGenerationResult singBoxConfig =
        context.appController->generateSingBoxTunConfig(profile);
    if (!singBoxConfig.success) {
        singBoxValidation.insert(QStringLiteral("attempted"), true);
        singBoxValidation.insert(QStringLiteral("success"), false);
        singBoxValidation.insert(QStringLiteral("stderrRedacted"),
                                 DiagnosticsRedactor::redactText(singBoxConfig.errorMessage,
                                                                 options.redactionMode));
        warn(snapshot, QStringLiteral("sing-box config generation failed"));
    } else {
        const QString configPath =
            QDir(AppPaths::testRuntimeDir())
                .filePath(QStringLiteral("diagnostics-singbox-config.json"));
        QFile file(configPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(singBoxConfig.config).toJson(QJsonDocument::Compact));
        }
        const CoreValidationResult result = context.coreManager->validateSingBoxConfig(
            AppSettings::instance().resolvedSingBoxPath(), configPath);
        singBoxValidation.insert(QStringLiteral("attempted"), true);
        singBoxValidation.insert(QStringLiteral("exitCode"), result.exitCode);
        singBoxValidation.insert(QStringLiteral("success"), result.success);
        singBoxValidation.insert(QStringLiteral("stdoutRedacted"),
                                 DiagnosticsRedactor::redactText(result.output, options.redactionMode));
        singBoxValidation.insert(QStringLiteral("stderrRedacted"),
                                 DiagnosticsRedactor::redactText(result.errorMessage,
                                                                 options.redactionMode));
        if (!result.success) {
            warn(snapshot, QStringLiteral("sing-box config validation failed"));
        }
    }
    root.insert(QStringLiteral("singBoxConfigValidation"), singBoxValidation);
    return root;
}

void collectConfigPreviews(const DiagnosticsOptions& options, const DiagnosticsContext& context,
                           DiagnosticsSnapshot* snapshot)
{
    if (!context.hasSelectedProfile || !context.xrayAdapter || !context.appController) {
        QJsonObject skipped;
        skipped.insert(QStringLiteral("generated"), false);
        skipped.insert(QStringLiteral("reason"), QStringLiteral("No profile selected"));
        putJson(snapshot, QStringLiteral("configs/xray-preview.redacted.json"), skipped);
        putJson(snapshot, QStringLiteral("configs/sing-box-preview.redacted.json"), skipped);
        return;
    }

    const Profile& profile = context.selectedProfile;
    const RoutingProfile routing = context.routingManager->activeProfile();
    const DnsProfile dns = context.dnsManager->activeProfile();

    const ConfigGenerationResult xrayConfig =
        context.xrayAdapter->generateConfig(profile, {}, routing, dns);
    if (xrayConfig.success) {
        putJson(snapshot, QStringLiteral("configs/xray-preview.redacted.json"),
                DiagnosticsRedactor::redactJsonObject(xrayConfig.config, options.redactionMode));
    } else {
        QJsonObject object;
        object.insert(QStringLiteral("generated"), false);
        object.insert(QStringLiteral("reason"), xrayConfig.errorMessage);
        putJson(snapshot, QStringLiteral("configs/xray-preview.redacted.json"), object);
    }

    const SingBoxConfigGenerationResult singBoxConfig =
        context.appController->generateSingBoxTunConfig(profile);
    if (singBoxConfig.success) {
        putJson(snapshot, QStringLiteral("configs/sing-box-preview.redacted.json"),
                DiagnosticsRedactor::redactJsonObject(singBoxConfig.config, options.redactionMode));
    } else {
        QJsonObject object;
        object.insert(QStringLiteral("generated"), false);
        object.insert(QStringLiteral("reason"), singBoxConfig.errorMessage);
        putJson(snapshot, QStringLiteral("configs/sing-box-preview.redacted.json"), object);
    }
}

QString collectRedactedLogs(const DiagnosticsOptions& options)
{
    const int maxLines = options.extendedLogs ? 5000 : 1000;
    QStringList lines;
    for (const QString& line : LogBuffer::instance().recentLines(maxLines)) {
        lines.append(DiagnosticsRedactor::redactLogLine(line, options.redactionMode));
    }
    return lines.join(QStringLiteral("\n"));
}

QJsonObject coreManagerEntry(const CoreInfo& info, bool requireChecksum)
{
    QJsonObject core;
    core.insert(QStringLiteral("managedPath"),
                DiagnosticsRedactor::redactPath(info.executablePath,
                                                DiagnosticsRedactionMode::Strict, false));
    core.insert(QStringLiteral("externalPath"), !info.managed);
    if (info.lastReleaseCheckAt.isValid()) {
        core.insert(QStringLiteral("lastReleaseCheck"),
                    info.lastReleaseCheckAt.toString(Qt::ISODate));
    }
    if (!info.selectedAssetName.isEmpty()) {
        core.insert(QStringLiteral("lastSelectedAsset"), info.selectedAssetName);
    }
    if (!info.lastUpdateError.isEmpty()) {
        core.insert(QStringLiteral("lastUpdateError"),
                    DiagnosticsRedactor::redactText(info.lastUpdateError,
                                                    DiagnosticsRedactionMode::Strict));
    } else if (!info.lastError.isEmpty() && info.status == CoreInstallStatus::Updating) {
        core.insert(QStringLiteral("lastUpdateError"),
                    DiagnosticsRedactor::redactText(info.lastError,
                                                    DiagnosticsRedactionMode::Strict));
    } else {
        core.insert(QStringLiteral("lastUpdateError"), QJsonValue::Null);
    }
    if (!info.lastReleaseCheckError.isEmpty()) {
        core.insert(QStringLiteral("lastReleaseCheckError"),
                    DiagnosticsRedactor::redactText(info.lastReleaseCheckError,
                                                    DiagnosticsRedactionMode::Strict));
    }
    core.insert(QStringLiteral("checksumPolicy"),
                requireChecksum ? QStringLiteral("require") : QStringLiteral("allow"));
    return core;
}

QJsonObject collectCoreManagerStatus(const DiagnosticsContext& context, DiagnosticsSnapshot* snapshot)
{
    QJsonObject root;
    QJsonObject manager;
    const bool requireChecksum = !AppSettings::instance().allowCoreUpdateWithoutChecksum();
    if (!context.coreBinaryManager) {
        collectorError(snapshot, QStringLiteral("coreManager"),
                       QStringLiteral("Core binary manager unavailable"));
        root.insert(QStringLiteral("coreManager"), manager);
        return root;
    }
    context.coreBinaryManager->refreshLocalState();
    for (const CoreInfo& info : context.coreBinaryManager->coreInfos()) {
        const QString key =
            info.type == CoreType::Xray ? QStringLiteral("xray") : QStringLiteral("singBox");
        manager.insert(key, coreManagerEntry(info, requireChecksum));
    }
    root.insert(QStringLiteral("coreManager"), manager);
    return root;
}

QJsonObject collectSubscriptionStatus(const DiagnosticsContext& context)
{
    QJsonObject root;
    QJsonObject subscriptions;
    int enabledCount = 0;
    QJsonArray errors;
    for (const Subscription& subscription : context.subscriptions) {
        if (subscription.enabled) {
            ++enabledCount;
        }
        if (!subscription.lastError.isEmpty()) {
            QJsonObject item;
            item.insert(QStringLiteral("name"), subscription.name);
            item.insert(QStringLiteral("url"), QStringLiteral("<redacted-url>"));
            item.insert(QStringLiteral("error"),
                        DiagnosticsRedactor::redactText(subscription.lastError,
                                                        DiagnosticsRedactionMode::Strict));
            errors.append(item);
        }
    }
    subscriptions.insert(QStringLiteral("count"), context.subscriptions.size());
    subscriptions.insert(QStringLiteral("enabled"), enabledCount);
    subscriptions.insert(QStringLiteral("lastUpdateErrors"), errors);
    root.insert(QStringLiteral("subscriptions"), subscriptions);
    return root;
}

QJsonObject collectFirstRunStatus()
{
    const AppSettings& settings = AppSettings::instance();
    QJsonObject root;
    QJsonObject firstRun;
    firstRun.insert(QStringLiteral("completed"), settings.firstRunCompleted());
    firstRun.insert(QStringLiteral("coreInstalledAtFirstRun"), settings.firstRunCoreInstalled());
    firstRun.insert(QStringLiteral("profilesImportedAtFirstRun"),
                    settings.firstRunProfilesImported());
    root.insert(QStringLiteral("firstRun"), firstRun);
    return root;
}

QJsonObject collectPackagingStatus()
{
    QJsonObject root;
    QJsonObject packaging;
    packaging.insert(QStringLiteral("portableMode"), AppPaths::isPortableMode());

    const QString manifestPath =
        QDir(AppPaths::applicationDir()).filePath(QStringLiteral("release-manifest.json"));
    packaging.insert(QStringLiteral("releaseManifestPresent"), QFile::exists(manifestPath));

    QJsonArray translations;
    const QStringList translationNames = {QStringLiteral("zarya_en.qm"),
                                          QStringLiteral("zarya_ru.qm")};
    for (const QString& name : translationNames) {
        bool present = QFile::exists(
            QDir(AppPaths::applicationDir()).filePath(QStringLiteral("translations/") + name));
#if defined(Q_OS_MACOS)
        present = present
                  || QFile::exists(QDir(AppPaths::applicationDir()).filePath(
                         QStringLiteral("../Resources/translations/") + name));
#endif
        if (!present) {
            continue;
        }
        if (name.contains(QStringLiteral("_en."))) {
            translations.append(QStringLiteral("en"));
        } else {
            translations.append(QStringLiteral("ru"));
        }
    }
    packaging.insert(QStringLiteral("translationsPresent"), translations);

    const QString publicBetaReadme =
        QDir(AppPaths::applicationDir()).filePath(QStringLiteral("docs/public-beta/README.md"));
#if defined(Q_OS_MACOS)
    const bool docsPresent =
        QFile::exists(publicBetaReadme)
        || QFile::exists(QDir(AppPaths::applicationDir()).filePath(
               QStringLiteral("../Resources/docs/public-beta/README.md")));
#else
    const bool docsPresent = QFile::exists(publicBetaReadme);
#endif
    packaging.insert(QStringLiteral("docsPresent"), docsPresent);
    root.insert(QStringLiteral("packaging"), packaging);
    return root;
}

QJsonObject collectRedactionReport(const DiagnosticsOptions& options)
{
    QJsonObject object;
    object.insert(QStringLiteral("mode"),
                  options.redactionMode == DiagnosticsRedactionMode::Strict ? QStringLiteral("strict")
                                                                            : QStringLiteral("basic"));
    object.insert(QStringLiteral("secretsIncluded"), false);
    QJsonArray fields;
    for (const QString& field : DiagnosticsRedactor::redactedFieldReport()) {
        fields.append(field);
    }
    object.insert(QStringLiteral("redactedFields"), fields);
    return object;
}

} // namespace

DiagnosticsSnapshot DiagnosticsCollector::collect(const DiagnosticsOptions& options,
                                                  const DiagnosticsContext& context,
                                                  LogCallback log)
{
    DiagnosticsSnapshot snapshot;
    DiagnosticsRedactor::resetReport();

    if (log) {
        log(QStringLiteral("Diagnostics collection started"));
    }

    if (categoryEnabled(options, DiagnosticsCategory::AppInfo)) {
        putJson(&snapshot, QStringLiteral("diagnostics/app-info.json"), collectAppInfo(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::PlatformInfo)) {
        putJson(&snapshot, QStringLiteral("diagnostics/platform-info.json"),
                collectPlatformInfo(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::Paths)) {
        putJson(&snapshot, QStringLiteral("diagnostics/paths.json"), collectPaths(options, context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::CoreStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/core-status.json"),
                collectCoreStatus(context, &snapshot));
        putJson(&snapshot, QStringLiteral("diagnostics/core-manager-status.json"),
                collectCoreManagerStatus(context, &snapshot));
    }
    if (categoryEnabled(options, DiagnosticsCategory::RuntimeStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/runtime-status.json"),
                collectRuntimeStatus(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::HelperStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/helper-status.json"),
                collectHelperStatus(context, &snapshot));
        putJson(&snapshot, QStringLiteral("diagnostics/helper-service-status.json"),
                collectHelperServiceStatus(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::SystemProxyStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/system-proxy-status.json"),
                collectSystemProxyStatus(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::KillSwitchStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/kill-switch-status.json"),
                collectKillSwitchStatus(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::RoutingDnsStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/routing-status.json"),
                collectRoutingStatus(context));
        putJson(&snapshot, QStringLiteral("diagnostics/dns-status.json"), collectDnsStatus(context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::GeoDataStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/geo-data-status.json"),
                collectGeoDataStatus(options, context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::RuleSetStatus)) {
        putJson(&snapshot, QStringLiteral("diagnostics/rule-set-status.json"),
                collectRuleSetStatus(options, context));
    }
    if (categoryEnabled(options, DiagnosticsCategory::ValidationOutput)) {
        putJson(&snapshot, QStringLiteral("diagnostics/validation-summary.json"),
                collectValidation(options, context, &snapshot));
    }
    if (categoryEnabled(options, DiagnosticsCategory::GeneratedConfigPreview)) {
        collectConfigPreviews(options, context, &snapshot);
    }
    if (categoryEnabled(options, DiagnosticsCategory::RecentLogs)) {
        putText(&snapshot, QStringLiteral("logs/app.redacted.log"), collectRedactedLogs(options));
    }
    if (categoryEnabled(options, DiagnosticsCategory::AppInfo)) {
        putJson(&snapshot, QStringLiteral("diagnostics/subscription-status.json"),
                collectSubscriptionStatus(context));
        putJson(&snapshot, QStringLiteral("diagnostics/first-run-status.json"),
                collectFirstRunStatus());
        putJson(&snapshot, QStringLiteral("diagnostics/packaging-status.json"),
                collectPackagingStatus());
    }
    if (categoryEnabled(options, DiagnosticsCategory::RecentErrors)) {
        QJsonObject errors;
        QJsonArray array = LogBuffer::instance().recentErrorsJson();
        for (int i = 0; i < array.size(); ++i) {
            QJsonObject item = array.at(i).toObject();
            item.insert(QStringLiteral("message"),
                        DiagnosticsRedactor::redactText(item.value(QStringLiteral("message"))
                                                          .toString(),
                                                      options.redactionMode));
            item.insert(QStringLiteral("detailsRedacted"),
                        DiagnosticsRedactor::redactText(item.value(QStringLiteral("message"))
                                                            .toString(),
                                                        options.redactionMode));
            array.replace(i, item);
        }
        errors.insert(QStringLiteral("errors"), array);
        putJson(&snapshot, QStringLiteral("diagnostics/recent-errors.json"), errors);
    }

    putJson(&snapshot, QStringLiteral("diagnostics/redaction-report.json"),
            collectRedactionReport(options));
    putText(&snapshot, QStringLiteral("raw/README.txt"),
            QStringLiteral("Raw unredacted runtime configs, helper tokens, and secrets are "
                           "intentionally not included."));

    if (log) {
        log(QStringLiteral("Diagnostics collection finished"));
    }
    return snapshot;
}

} // namespace zarya
