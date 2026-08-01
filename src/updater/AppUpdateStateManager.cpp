#include "updater/AppUpdateStateManager.h"

#include "diagnostics/DiagnosticsRedactor.h"
#include "ui/desktopapp/UiMessagePresenter.h"
#include "updater/AppUpdatePaths.h"
#include "updater/runner/UpdatePlanFile.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

namespace zarya {

namespace {

QJsonObject readJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

} // namespace

AppUpdateStartupNotice AppUpdateStateManager::checkStartupState()
{
    AppUpdateStartupNotice notice;

    if (QFile::exists(AppUpdatePaths::updateSuccessPath())) {
        const QJsonObject object = readJsonFile(AppUpdatePaths::updateSuccessPath());
        notice.kind = AppUpdateStartupNotice::Kind::Success;
        notice.targetVersion = object.value(QStringLiteral("targetVersion")).toString();
        notice.previousVersion = object.value(QStringLiteral("previousVersion")).toString();
        return notice;
    }

    if (QFile::exists(AppUpdatePaths::updateFailedPath())) {
        const QJsonObject object = readJsonFile(AppUpdatePaths::updateFailedPath());
        notice.kind = AppUpdateStartupNotice::Kind::Failed;
        notice.targetVersion = object.value(QStringLiteral("targetVersion")).toString();
        notice.previousVersion = object.value(QStringLiteral("previousVersion")).toString();
        notice.reason = object.value(QStringLiteral("reason")).toString();
        notice.backupDir = object.value(QStringLiteral("backupDir")).toString();
        return notice;
    }

    if (QFile::exists(AppUpdatePaths::pendingUpdatePath())) {
        UpdatePlan plan;
        QString error;
        if (UpdatePlan::readFile(AppUpdatePaths::pendingUpdatePath(), &plan, &error)) {
            const QDateTime created = QDateTime::fromString(plan.createdAt, Qt::ISODate);
            if (created.isValid()
                && created.secsTo(QDateTime::currentDateTimeUtc()) > 24 * 3600) {
                notice.kind = AppUpdateStartupNotice::Kind::StalePending;
                notice.targetVersion = plan.targetVersion;
            }
        }
    }

    return notice;
}

void AppUpdateStateManager::cleanupAfterSuccessNotice()
{
    QFile::remove(AppUpdatePaths::updateSuccessPath());
    QFile::remove(AppUpdatePaths::pendingUpdatePath());

    const QString stagingRoot = AppUpdatePaths::stagingRootDir();
    QDir stagingDir(stagingRoot);
    const QStringList entries = stagingDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        if (entry.startsWith(QStringLiteral(".extract-"))) {
            continue;
        }
        QDir(stagingDir.filePath(entry)).removeRecursively();
    }
}

void AppUpdateStateManager::showStartupNotice(QWidget* parent,
                                              const AppUpdateStartupNotice& notice)
{
    switch (notice.kind) {
    case AppUpdateStartupNotice::Kind::Success: {
        const QString message =
            notice.targetVersion.isEmpty()
                ? QObject::tr("Zarya was updated successfully.")
                : QObject::tr("Zarya was updated to %1.").arg(notice.targetVersion);
        UiMessagePresenter::information(parent, QObject::tr("App Update"), message);
        cleanupAfterSuccessNotice();
        break;
    }
    case AppUpdateStartupNotice::Kind::Failed: {
        QString message = QObject::tr("The app update failed and was rolled back.");
        if (!notice.reason.isEmpty()) {
            message += QStringLiteral("\n\n") + notice.reason;
        }
        const QString selected = UiMessagePresenter::choose(
            parent,
            QObject::tr("App Update"),
            message,
            UiMessageTone::Warning,
            {
                {QStringLiteral("show-log"), QObject::tr("Show Log"),
                 UiMessageActionRole::Primary, true, false},
                {QStringLiteral("open-folder"), QObject::tr("Open Update Folder"),
                 UiMessageActionRole::Secondary, false, false},
                {QStringLiteral("ok"), QObject::tr("OK"),
                 UiMessageActionRole::Secondary, false, true},
            });
        if (selected == QStringLiteral("show-log")) {
            const QString logPath =
                QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(QStringLiteral("last-update.log"));
            if (QFile::exists(logPath)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
            }
        } else if (selected == QStringLiteral("open-folder")) {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(AppUpdatePaths::appUpdatesRootDir()));
        }
        break;
    }
    case AppUpdateStartupNotice::Kind::StalePending:
        UiMessagePresenter::warning(
            parent, QObject::tr("App Update"),
            QObject::tr("A previous update attempt was not completed. The pending update plan "
                        "for %1 is stale.")
                .arg(notice.targetVersion));
        break;
    case AppUpdateStartupNotice::Kind::None:
        break;
    }
}

QString AppUpdateStateManager::readLastUpdaterLogSummary()
{
    const QString logPath =
        QDir(AppUpdatePaths::appUpdatesRootDir()).filePath(QStringLiteral("last-update.log"));
    if (!QFile::exists(logPath)) {
        return {};
    }
    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QString content = QString::fromUtf8(file.readAll());
    const QStringList lines =
        content.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    const int start = qMax(0, lines.size() - 8);
    return DiagnosticsRedactor::redactText(lines.mid(start).join(QStringLiteral("\n")),
                                          DiagnosticsRedactionMode::Strict);
}

QJsonObject AppUpdateStateManager::lastStateJson()
{
    if (QFile::exists(AppUpdatePaths::updateSuccessPath())) {
        return readJsonFile(AppUpdatePaths::updateSuccessPath());
    }
    if (QFile::exists(AppUpdatePaths::updateFailedPath())) {
        return readJsonFile(AppUpdatePaths::updateFailedPath());
    }
    return {};
}

} // namespace zarya
