#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

enum class AppUpdateChannel {
    Dev,
    Beta,
    Stable,
};

class AppUpdateChannelPolicy {
public:
    static AppUpdateChannel fromString(const QString& text);
    static QString toString(AppUpdateChannel channel);
    static AppUpdateChannel defaultChannelFromBuild();
    static QStringList allowedManifestChannelKeys(AppUpdateChannel channel);
    static bool canSeeChannel(AppUpdateChannel userChannel, const QString& manifestChannelKey);
};

} // namespace zarya
