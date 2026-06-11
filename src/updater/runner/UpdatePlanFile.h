#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace zarya {

struct UpdatePlanPostUpdateCheck {
    QString command;
    QStringList args;
    QString expectedVersion;
};

struct UpdatePlan {
    QString format = QStringLiteral("zarya-update-plan");
    int formatVersion = 1;
    QString createdAt;

    QString currentVersion;
    QString targetVersion;

    QString installationMode;
    QString platform;
    QString architecture;

    QString appDir;
    QString stagingDir;
    QString backupDir;

    QStringList preservePaths;

    QString mainExecutable;
    QString helperExecutable;
    QString updaterExecutable;

    UpdatePlanPostUpdateCheck postUpdateCheck;

    bool restartAfterUpdate = true;
    QStringList restartArgs;

    bool isValid(QString* errorMessage = nullptr) const;
    QJsonObject toJson() const;
    static UpdatePlan fromJson(const QJsonObject& object, QString* errorMessage = nullptr);
    static bool readFile(const QString& path, UpdatePlan* plan, QString* errorMessage = nullptr);
    static bool writeFile(const QString& path, const UpdatePlan& plan, QString* errorMessage = nullptr);

    static QStringList defaultPreservePaths();
    static QString mainExecutableName();
    static QString helperExecutableName();
    static QString updaterExecutableName();
};

} // namespace zarya
