#pragma once

#include <QString>
#include <functional>

namespace zarya {

using KillSwitchLogSink = std::function<void(const QString&)>;

void setKillSwitchLogSink(KillSwitchLogSink sink);
void killSwitchLog(const QString& line);

} // namespace zarya
