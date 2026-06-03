#include "domain/DnsServer.h"

#include <QUuid>

namespace zarya {

DnsServer DnsServer::createDefault()
{
    DnsServer server;
    server.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    server.enabled = true;
    server.kind = DnsServerKind::PlainIp;
    return server;
}

QString dnsServerKindToString(DnsServerKind kind)
{
    switch (kind) {
    case DnsServerKind::PlainIp:
        return QStringLiteral("plain");
    case DnsServerKind::DoH:
        return QStringLiteral("doh");
    case DnsServerKind::Local:
        return QStringLiteral("local");
    case DnsServerKind::FakeDnsPlaceholder:
        return QStringLiteral("fakedns");
    }
    return QStringLiteral("plain");
}

DnsServerKind dnsServerKindFromString(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("doh")) {
        return DnsServerKind::DoH;
    }
    if (normalized == QStringLiteral("local")) {
        return DnsServerKind::Local;
    }
    if (normalized == QStringLiteral("fakedns")) {
        return DnsServerKind::FakeDnsPlaceholder;
    }
    return DnsServerKind::PlainIp;
}

QString dnsServerKindDisplayString(DnsServerKind kind)
{
    switch (kind) {
    case DnsServerKind::PlainIp:
        return QStringLiteral("Plain");
    case DnsServerKind::DoH:
        return QStringLiteral("DoH");
    case DnsServerKind::Local:
        return QStringLiteral("Local");
    case DnsServerKind::FakeDnsPlaceholder:
        return QStringLiteral("FakeDNS");
    }
    return QStringLiteral("Plain");
}

} // namespace zarya
