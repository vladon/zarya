#include "killswitch/windows/WindowsWfpUtils.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QUuid>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winsvc.h>

#include <cstring>

namespace zarya {

QString wfpGuidToString(const GUID& guid)
{
    return QUuid(guid).toString(QUuid::WithoutBraces);
}

QString wfpFormatError(unsigned long errorCode)
{
    if (errorCode == 0) {
        return QStringLiteral("Unknown WFP error.");
    }
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                        | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length =
        FormatMessageW(flags, nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    QString message;
    if (length > 0 && buffer != nullptr) {
        message = QString::fromWCharArray(buffer).trimmed();
    }
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    if (message.isEmpty()) {
        message = QStringLiteral("WFP error 0x%1").arg(errorCode, 0, 16);
    }
    return message;
}

bool wfpIsBfeServiceRunning(QString* errorMessage)
{
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(GetLastError());
        }
        return false;
    }

    SC_HANDLE service = OpenServiceW(manager, L"BFE", SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        if (errorMessage) {
            *errorMessage = QStringLiteral("Base Filtering Engine (BFE) service is not available.");
        }
        return false;
    }

    SERVICE_STATUS status = {};
    const BOOL ok = QueryServiceStatus(service, &status);
    CloseServiceHandle(service);
    CloseServiceHandle(manager);

    if (!ok) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(GetLastError());
        }
        return false;
    }

    if (status.dwCurrentState != SERVICE_RUNNING) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Base Filtering Engine (BFE) service is not running.");
        }
        return false;
    }

    return true;
}

bool wfpParseIpv4(const QString& address, UINT32* hostOrderAddress)
{
    if (!hostOrderAddress) {
        return false;
    }
    const QHostAddress host(address);
    if (host.protocol() != QAbstractSocket::IPv4Protocol) {
        return false;
    }
    *hostOrderAddress = host.toIPv4Address();
    return true;
}

bool wfpParseIpv6(const QString& address, UINT8 outAddress[16])
{
    if (!outAddress) {
        return false;
    }
    const QHostAddress host(address);
    if (host.protocol() != QAbstractSocket::IPv6Protocol) {
        return false;
    }
    const Q_IPV6ADDR bytes = host.toIPv6Address();
    memcpy(outAddress, bytes.c, sizeof(bytes.c));
    return true;
}

} // namespace zarya
