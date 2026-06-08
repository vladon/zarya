#pragma once

#include <QString>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace zarya {

QString wfpGuidToString(const GUID& guid);
bool wfpIsBfeServiceRunning(QString* errorMessage);
QString wfpFormatError(unsigned long errorCode);
bool wfpParseIpv4(const QString& address, UINT32* hostOrderAddress);
bool wfpParseIpv6(const QString& address, UINT8 outAddress[16]);

} // namespace zarya
