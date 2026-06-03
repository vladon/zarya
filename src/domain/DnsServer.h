#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

enum class DnsServerKind {
    PlainIp,
    DoH,
    Local,
    FakeDnsPlaceholder,
};

struct DnsServer {
    QString id;
    bool enabled = true;
    DnsServerKind kind = DnsServerKind::PlainIp;
    QString address;
    int port = 0;
    QStringList domains;
    QStringList expectIPs;
    QString queryStrategy;
    int timeoutMs = 0;
    QString tag;
    bool skipFallback = false;
    QString note;

    static DnsServer createDefault();
};

QString dnsServerKindToString(DnsServerKind kind);
DnsServerKind dnsServerKindFromString(const QString& value);
QString dnsServerKindDisplayString(DnsServerKind kind);

} // namespace zarya
