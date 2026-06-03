#pragma once

#include "domain/Profile.h"

#include <QJsonObject>
#include <QString>

namespace zarya::testhelpers {

Profile sampleVlessRealityProfile();
Profile sampleVmessTcpTlsProfile();
Profile sampleTrojanTlsProfile();
Profile sampleShadowsocksProfile();
QJsonObject sampleVlessRealityProxyOutbound();
QString generateXrayConfigJson(const Profile& profile, QString* errorMessage = nullptr);
bool proxyOutboundHasReality(const QJsonObject& proxyOutbound);
bool jsonContainsKeys(const QJsonObject& object, const QStringList& keys, QString* missingKey);

} // namespace zarya::testhelpers
