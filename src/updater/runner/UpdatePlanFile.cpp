#include "updater/runner/UpdatePlanFile.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace zarya {

namespace {

QString readRequiredString(const QJsonObject& object, const char* key, QString* errorMessage)
{
    const QString value = object.value(QString::fromLatin1(key)).toString().trimmed();
    if (value.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan missing required field: %1").arg(
                QString::fromLatin1(key));
        }
    }
    return value;
}

QStringList readStringList(const QJsonObject& object, const char* key)
{
    QStringList values;
    const QJsonValue raw = object.value(QString::fromLatin1(key));
    if (!raw.isArray()) {
        return values;
    }
    for (const QJsonValue& item : raw.toArray()) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            values.append(text);
        }
    }
    return values;
}

} // namespace

QString UpdatePlan::mainExecutableName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("Zarya.exe");
#else
    return QStringLiteral("zarya");
#endif
}

QString UpdatePlan::helperExecutableName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("zarya-helper.exe");
#else
    return QStringLiteral("zarya-helper");
#endif
}

QString UpdatePlan::updaterExecutableName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("zarya-updater.exe");
#else
    return QStringLiteral("zarya-updater");
#endif
}

QStringList UpdatePlan::defaultPreservePaths()
{
    return {QStringLiteral("data"), QStringLiteral("runtime"), QStringLiteral("portable.flag"),
            QStringLiteral("cores")};
}

bool UpdatePlan::isValid(QString* errorMessage) const
{
    if (format != QStringLiteral("zarya-update-plan") || formatVersion != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported update plan format.");
        }
        return false;
    }
    if (createdAt.isEmpty() || currentVersion.isEmpty() || targetVersion.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan is missing version metadata.");
        }
        return false;
    }
    if (installationMode != QStringLiteral("Portable")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan installation mode must be Portable.");
        }
        return false;
    }
    if (appDir.isEmpty() || stagingDir.isEmpty() || backupDir.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan is missing directory paths.");
        }
        return false;
    }
    if (mainExecutable.isEmpty() || helperExecutable.isEmpty() || updaterExecutable.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan is missing executable names.");
        }
        return false;
    }
    if (postUpdateCheck.command.isEmpty() || postUpdateCheck.expectedVersion.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan is missing post-update check.");
        }
        return false;
    }
    return true;
}

QJsonObject UpdatePlan::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("format"), format);
    object.insert(QStringLiteral("formatVersion"), formatVersion);
    object.insert(QStringLiteral("createdAt"), createdAt);
    object.insert(QStringLiteral("currentVersion"), currentVersion);
    object.insert(QStringLiteral("targetVersion"), targetVersion);
    object.insert(QStringLiteral("installationMode"), installationMode);
    object.insert(QStringLiteral("platform"), platform);
    object.insert(QStringLiteral("architecture"), architecture);
    object.insert(QStringLiteral("appDir"), QDir::fromNativeSeparators(appDir));
    object.insert(QStringLiteral("stagingDir"), QDir::fromNativeSeparators(stagingDir));
    object.insert(QStringLiteral("backupDir"), QDir::fromNativeSeparators(backupDir));

    QJsonArray preserve;
    for (const QString& path : preservePaths) {
        preserve.append(path);
    }
    object.insert(QStringLiteral("preservePaths"), preserve);

    object.insert(QStringLiteral("mainExecutable"), mainExecutable);
    object.insert(QStringLiteral("helperExecutable"), helperExecutable);
    object.insert(QStringLiteral("updaterExecutable"), updaterExecutable);

    QJsonObject check;
    check.insert(QStringLiteral("command"), postUpdateCheck.command);
    QJsonArray args;
    for (const QString& arg : postUpdateCheck.args) {
        args.append(arg);
    }
    check.insert(QStringLiteral("args"), args);
    check.insert(QStringLiteral("expectedVersion"), postUpdateCheck.expectedVersion);
    object.insert(QStringLiteral("postUpdateCheck"), check);

    object.insert(QStringLiteral("restartAfterUpdate"), restartAfterUpdate);
    QJsonArray restartArgsJson;
    for (const QString& arg : restartArgs) {
        restartArgsJson.append(arg);
    }
    object.insert(QStringLiteral("restartArgs"), restartArgsJson);
    return object;
}

UpdatePlan UpdatePlan::fromJson(const QJsonObject& object, QString* errorMessage)
{
    UpdatePlan plan;
    plan.format = object.value(QStringLiteral("format")).toString(plan.format);
    plan.formatVersion = object.value(QStringLiteral("formatVersion")).toInt(plan.formatVersion);
    plan.createdAt = readRequiredString(object, "createdAt", errorMessage);
    plan.currentVersion = readRequiredString(object, "currentVersion", errorMessage);
    plan.targetVersion = readRequiredString(object, "targetVersion", errorMessage);
    plan.installationMode = readRequiredString(object, "installationMode", errorMessage);
    plan.platform = object.value(QStringLiteral("platform")).toString();
    plan.architecture = object.value(QStringLiteral("architecture")).toString();
    plan.appDir = QDir::fromNativeSeparators(readRequiredString(object, "appDir", errorMessage));
    plan.stagingDir =
        QDir::fromNativeSeparators(readRequiredString(object, "stagingDir", errorMessage));
    plan.backupDir =
        QDir::fromNativeSeparators(readRequiredString(object, "backupDir", errorMessage));

    plan.preservePaths = readStringList(object, "preservePaths");
    if (plan.preservePaths.isEmpty()) {
        plan.preservePaths = defaultPreservePaths();
    }

    plan.mainExecutable = readRequiredString(object, "mainExecutable", errorMessage);
    plan.helperExecutable = readRequiredString(object, "helperExecutable", errorMessage);
    plan.updaterExecutable = readRequiredString(object, "updaterExecutable", errorMessage);

    const QJsonObject check = object.value(QStringLiteral("postUpdateCheck")).toObject();
    plan.postUpdateCheck.command = check.value(QStringLiteral("command")).toString();
    plan.postUpdateCheck.args = readStringList(check, "args");
    plan.postUpdateCheck.expectedVersion = check.value(QStringLiteral("expectedVersion")).toString();

    plan.restartAfterUpdate = object.value(QStringLiteral("restartAfterUpdate")).toBool(true);
    plan.restartArgs = readStringList(object, "restartArgs");
    if (plan.restartArgs.isEmpty()) {
        plan.restartArgs = {QStringLiteral("--post-update")};
    }

    if (errorMessage && !errorMessage->isEmpty()) {
        return plan;
    }
    if (!plan.isValid(errorMessage)) {
        return plan;
    }
    return plan;
}

bool UpdatePlan::readFile(const QString& path, UpdatePlan* plan, QString* errorMessage)
{
    if (plan == nullptr) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan output pointer is null.");
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to read update plan: %1").arg(path);
        }
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Update plan is not valid JSON.");
        }
        return false;
    }

    QString parseError;
    *plan = fromJson(doc.object(), &parseError);
    if (!parseError.isEmpty() || !plan->isValid(&parseError)) {
        if (errorMessage) {
            *errorMessage = parseError;
        }
        return false;
    }
    return true;
}

bool UpdatePlan::writeFile(const QString& path, const UpdatePlan& plan, QString* errorMessage)
{
    QString validationError;
    UpdatePlan validated = plan;
    if (validated.createdAt.isEmpty()) {
        validated.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
    if (!validated.isValid(&validationError)) {
        if (errorMessage) {
            *errorMessage = validationError;
        }
        return false;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write update plan: %1").arg(path);
        }
        return false;
    }

    const QJsonDocument doc(validated.toJson());
    if (file.write(doc.toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write update plan bytes.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
