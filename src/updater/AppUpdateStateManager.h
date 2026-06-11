#pragma once

#include <QJsonObject>
#include <QString>

class QWidget;

namespace zarya {

struct AppUpdateStartupNotice {
    enum class Kind {
        None,
        Success,
        Failed,
        StalePending,
    };

    Kind kind = Kind::None;
    QString targetVersion;
    QString previousVersion;
    QString reason;
    QString backupDir;
};

class AppUpdateStateManager {
public:
    static AppUpdateStartupNotice checkStartupState();
    static void cleanupAfterSuccessNotice();
    static void showStartupNotice(QWidget* parent, const AppUpdateStartupNotice& notice);
    static QString readLastUpdaterLogSummary();
    static QJsonObject lastStateJson();
};

} // namespace zarya
