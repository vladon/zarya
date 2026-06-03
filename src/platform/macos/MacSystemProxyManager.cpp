#include "platform/macos/MacSystemProxyManager.h"

#include "platform/PlatformProcessUtils.h"
#include "storage/AppSettings.h"

#include <QRegularExpression>

namespace zarya {

namespace {

QVariantMap serviceStateToVariant(const MacSystemProxyManager::ServiceProxySnapshot& web,
                                  const MacSystemProxyManager::ServiceProxySnapshot& secure,
                                  const QStringList& bypass)
{
    QVariantMap map;
    map.insert(QStringLiteral("webEnabled"), web.enabled);
    map.insert(QStringLiteral("webServer"), web.server);
    map.insert(QStringLiteral("webPort"), web.port);
    map.insert(QStringLiteral("secureEnabled"), secure.enabled);
    map.insert(QStringLiteral("secureServer"), secure.server);
    map.insert(QStringLiteral("securePort"), secure.port);
    map.insert(QStringLiteral("bypassDomains"), bypass);
    return map;
}

} // namespace

bool MacSystemProxyManager::isSupported() const
{
    const ProcessResult result =
        runProcess(QStringLiteral("networksetup"), {QStringLiteral("-listallnetworkservices")});
    return result.success;
}

QString MacSystemProxyManager::backendName() const
{
    return QStringLiteral("macOS networksetup");
}

QString MacSystemProxyManager::supportLevel() const
{
    return isSupported() ? QStringLiteral("full") : QStringLiteral("unsupported");
}

QString MacSystemProxyManager::limitations() const
{
    return QStringLiteral(
        "Uses networksetup for HTTP/HTTPS proxy on selected network services. May require "
        "administrator privileges on some systems. Not all applications respect macOS system "
        "proxy settings.");
}

bool MacSystemProxyManager::runNetworkSetup(const QStringList& arguments, QString* errorMessage,
                                            QString* standardOutput)
{
    const ProcessResult result = runProcess(QStringLiteral("networksetup"), arguments);
    if (standardOutput) {
        *standardOutput = result.standardOutput;
    }
    if (!result.success) {
        if (errorMessage) {
            const QString detail = result.errorMessage.isEmpty() ? result.standardError
                                                                   : result.errorMessage;
            *errorMessage =
                QStringLiteral("networksetup failed (%1): %2")
                    .arg(arguments.join(QLatin1Char(' ')), detail);
        }
        return false;
    }
    return true;
}

MacSystemProxyManager::ServiceProxySnapshot MacSystemProxyManager::parseProxyOutput(
    const QString& output)
{
    ServiceProxySnapshot snapshot;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        const QString key = line.left(colon).trimmed();
        const QString value = line.mid(colon + 1).trimmed();
        if (key == QStringLiteral("Enabled")) {
            snapshot.enabled = value.compare(QStringLiteral("Yes"), Qt::CaseInsensitive) == 0;
        } else if (key == QStringLiteral("Server")) {
            snapshot.server = value;
        } else if (key == QStringLiteral("Port")) {
            snapshot.port = value.toInt();
        }
    }
    return snapshot;
}

QStringList MacSystemProxyManager::listNetworkServices(QString* errorMessage) const
{
    QString output;
    if (!runNetworkSetup({QStringLiteral("-listallnetworkservices")}, errorMessage, &output)) {
        return {};
    }

    QStringList services;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('*'))
            || trimmed.startsWith(QStringLiteral("An asterisk"))) {
            continue;
        }
        services.append(trimmed);
    }
    return services;
}

QStringList MacSystemProxyManager::servicesToManage(QString* errorMessage) const
{
    const AppSettings& settings = AppSettings::instance();
    const QString preferred = settings.macPreferredNetworkService().trimmed();
    if (!preferred.isEmpty()) {
        return {preferred};
    }

    const QStringList allServices = listNetworkServices(errorMessage);
    if (allServices.isEmpty()) {
        return {};
    }

    if (settings.macApplyProxyToAllServices()) {
        return allServices;
    }

    QString orderOutput;
    if (runNetworkSetup({QStringLiteral("-listnetworkserviceorder")}, errorMessage, &orderOutput)) {
        static const QRegularExpression servicePattern(QStringLiteral("\\(Hardware Port: ([^,]+)"));
        QRegularExpressionMatchIterator it = servicePattern.globalMatch(orderOutput);
        while (it.hasNext()) {
            const QString hardwarePort = it.next().captured(1).trimmed();
            for (const QString& service : allServices) {
                if (service.compare(hardwarePort, Qt::CaseInsensitive) == 0
                    || service.contains(hardwarePort, Qt::CaseInsensitive)) {
                    return {service};
                }
            }
        }
    }

    return {allServices.first()};
}

MacSystemProxyManager::ServiceProxySnapshot MacSystemProxyManager::readWebProxy(
    const QString& service, QString* errorMessage) const
{
    QString output;
    if (!runNetworkSetup({QStringLiteral("-getwebproxy"), service}, errorMessage, &output)) {
        return {};
    }
    return parseProxyOutput(output);
}

MacSystemProxyManager::ServiceProxySnapshot MacSystemProxyManager::readSecureWebProxy(
    const QString& service, QString* errorMessage) const
{
    QString output;
    if (!runNetworkSetup({QStringLiteral("-getsecurewebproxy"), service}, errorMessage, &output)) {
        return {};
    }
    return parseProxyOutput(output);
}

QStringList MacSystemProxyManager::readBypassDomains(const QString& service,
                                                     QString* errorMessage) const
{
    QString output;
    if (!runNetworkSetup({QStringLiteral("-getproxybypassdomains"), service}, errorMessage,
                         &output)) {
        return {};
    }

    if (output.contains(QStringLiteral("aren't any"), Qt::CaseInsensitive)
        || output.contains(QStringLiteral("There are no"), Qt::CaseInsensitive)) {
        return {};
    }

    QStringList domains;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            domains.append(trimmed);
        }
    }
    return domains;
}

SystemProxyState MacSystemProxyManager::readCurrentState(QString* errorMessage)
{
    SystemProxyState state;
    state.platform = QStringLiteral("macos");
    state.backend = backendName();
    state.supportLevel = supportLevel();

    const QStringList services = servicesToManage(errorMessage);
    state.affectedNetworkServices = services;
    if (!services.isEmpty()) {
        state.activeNetworkService = services.first();
    }

    QVariantMap servicesMap;
    for (const QString& service : services) {
        QString serviceError;
        const ServiceProxySnapshot web = readWebProxy(service, &serviceError);
        if (!serviceError.isEmpty() && errorMessage) {
            *errorMessage = serviceError;
            return state;
        }
        const ServiceProxySnapshot secure = readSecureWebProxy(service, &serviceError);
        if (!serviceError.isEmpty() && errorMessage) {
            *errorMessage = serviceError;
            return state;
        }
        const QStringList bypass = readBypassDomains(service, &serviceError);
        if (!serviceError.isEmpty() && errorMessage) {
            *errorMessage = serviceError;
            return state;
        }
        servicesMap.insert(service, serviceStateToVariant(web, secure, bypass));
    }

    state.rawValues.insert(QStringLiteral("services"), servicesMap);

    if (!services.isEmpty()) {
        const ServiceProxySnapshot web = readWebProxy(services.first(), nullptr);
        state.proxyEnabled = web.enabled;
        if (web.port > 0) {
            state.proxyServer = QStringLiteral("%1:%2").arg(web.server).arg(web.port);
        }
    }

    return state;
}

bool MacSystemProxyManager::applyProxyToService(const QString& service, const QString& host,
                                                int port, QString* errorMessage)
{
    const QString portString = QString::number(port);
    if (!runNetworkSetup({QStringLiteral("-setwebproxy"), service, host, portString}, errorMessage)) {
        return false;
    }
    if (!runNetworkSetup({QStringLiteral("-setsecurewebproxy"), service, host, portString},
                         errorMessage)) {
        return false;
    }
    if (!runNetworkSetup({QStringLiteral("-setwebproxystate"), service, QStringLiteral("on")},
                         errorMessage)) {
        return false;
    }
    if (!runNetworkSetup(
            {QStringLiteral("-setsecurewebproxystate"), service, QStringLiteral("on")},
            errorMessage)) {
        return false;
    }
    if (!runNetworkSetup({QStringLiteral("-setproxybypassdomains"), service, QStringLiteral("localhost"),
                          QStringLiteral("127.0.0.1"), QStringLiteral("::1"),
                          QStringLiteral("*.local")},
                         errorMessage)) {
        return false;
    }
    return true;
}

bool MacSystemProxyManager::applyHttpProxy(const QString& host, int port, QString* errorMessage)
{
    if (port < 1 || port > 65535) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid HTTP proxy port: %1").arg(port);
        }
        return false;
    }

    const QStringList services = servicesToManage(errorMessage);
    if (services.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No macOS network services available for proxy.");
        }
        return false;
    }

    for (const QString& service : services) {
        if (!applyProxyToService(service, host, port, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool MacSystemProxyManager::restoreService(const QString& service,
                                           const QVariantMap& serviceState,
                                           QString* errorMessage)
{
    ServiceProxySnapshot web;
    web.enabled = serviceState.value(QStringLiteral("webEnabled")).toBool();
    web.server = serviceState.value(QStringLiteral("webServer")).toString();
    web.port = serviceState.value(QStringLiteral("webPort")).toInt();

    ServiceProxySnapshot secure;
    secure.enabled = serviceState.value(QStringLiteral("secureEnabled")).toBool();
    secure.server = serviceState.value(QStringLiteral("secureServer")).toString();
    secure.port = serviceState.value(QStringLiteral("securePort")).toInt();
    const QStringList bypass = serviceState.value(QStringLiteral("bypassDomains")).toStringList();

    if (web.enabled && !web.server.isEmpty() && web.port > 0) {
        if (!runNetworkSetup({QStringLiteral("-setwebproxy"), service, web.server,
                              QString::number(web.port)},
                             errorMessage)) {
            return false;
        }
        if (!runNetworkSetup({QStringLiteral("-setwebproxystate"), service, QStringLiteral("on")},
                             errorMessage)) {
            return false;
        }
    } else {
        if (!runNetworkSetup({QStringLiteral("-setwebproxystate"), service, QStringLiteral("off")},
                             errorMessage)) {
            return false;
        }
    }

    if (secure.enabled && !secure.server.isEmpty() && secure.port > 0) {
        if (!runNetworkSetup({QStringLiteral("-setsecurewebproxy"), service, secure.server,
                              QString::number(secure.port)},
                             errorMessage)) {
            return false;
        }
        if (!runNetworkSetup(
                {QStringLiteral("-setsecurewebproxystate"), service, QStringLiteral("on")},
                errorMessage)) {
            return false;
        }
    } else {
        if (!runNetworkSetup(
                {QStringLiteral("-setsecurewebproxystate"), service, QStringLiteral("off")},
                errorMessage)) {
            return false;
        }
    }

    QStringList bypassArgs = {QStringLiteral("-setproxybypassdomains"), service};
    if (bypass.isEmpty()) {
        bypassArgs.append(QStringLiteral("Empty"));
    } else {
        bypassArgs.append(bypass);
    }
    return runNetworkSetup(bypassArgs, errorMessage);
}

bool MacSystemProxyManager::restoreState(const SystemProxyState& state, QString* errorMessage)
{
    const QVariantMap servicesMap = state.rawValues.value(QStringLiteral("services")).toMap();
    if (servicesMap.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Missing macOS service proxy snapshot for restore.");
        }
        return false;
    }

    for (auto it = servicesMap.begin(); it != servicesMap.end(); ++it) {
        if (!restoreService(it.key(), it.value().toMap(), errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace zarya
