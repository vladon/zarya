#include "killswitch/KillSwitchLog.h"

namespace zarya {

namespace {

KillSwitchLogSink g_sink;

} // namespace

void setKillSwitchLogSink(KillSwitchLogSink sink)
{
    g_sink = std::move(sink);
}

void killSwitchLog(const QString& line)
{
    if (g_sink) {
        g_sink(line);
    }
}

} // namespace zarya
