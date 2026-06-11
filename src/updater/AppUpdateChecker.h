#pragma once

#include "updater/AppUpdatePlanner.h"

#include <QObject>
#include <QString>
#include <QUrl>

namespace zarya {

class AppUpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit AppUpdateChecker(QObject* parent = nullptr);

    void setLocalManifestPath(const QString& path);
    QString localManifestPath() const;

    void checkForUpdates();

signals:
    void updateCheckStarted();
    void updateCheckFinished(const AppUpdatePlan& plan);
    void updateCheckFailed(const QString& error);

private:
    bool loadManifestFromFile(const QString& path, AppUpdateManifest* manifest,
                              QString* errorMessage);
    bool loadManifestFromUrl(const QUrl& url, AppUpdateManifest* manifest, QString* errorMessage);

    QString m_localManifestPath;
};

} // namespace zarya
