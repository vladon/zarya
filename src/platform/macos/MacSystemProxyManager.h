#pragma once

#include "platform/ISystemProxyManager.h"

#include <QStringList>

namespace zarya {

class MacSystemProxyManager : public ISystemProxyManager {
public:
    bool isSupported() const override;
    SystemProxyState readCurrentState(QString* errorMessage = nullptr) override;
    bool applyHttpProxy(const QString& host, int port, QString* errorMessage = nullptr) override;
    bool restoreState(const SystemProxyState& state, QString* errorMessage = nullptr) override;

    QString backendName() const override;
    QString supportLevel() const override;
    QString limitations() const override;

    struct ServiceProxySnapshot {
        bool enabled = false;
        QString server;
        int port = 0;
        QStringList bypassDomains;
    };

private:
    QStringList listNetworkServices(QString* errorMessage) const;
    QStringList servicesToManage(QString* errorMessage) const;
    ServiceProxySnapshot readWebProxy(const QString& service, QString* errorMessage) const;
    ServiceProxySnapshot readSecureWebProxy(const QString& service, QString* errorMessage) const;
    QStringList readBypassDomains(const QString& service, QString* errorMessage) const;

    bool applyProxyToService(const QString& service, const QString& host, int port,
                             QString* errorMessage);
    bool restoreService(const QString& service, const QVariantMap& serviceState,
                        QString* errorMessage);

    static ServiceProxySnapshot parseProxyOutput(const QString& output);
    static bool runNetworkSetup(const QStringList& arguments, QString* errorMessage,
                                QString* standardOutput = nullptr);
};

} // namespace zarya
