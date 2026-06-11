#include "app/BuildInfo.h"

#include "BuildInfo_config.h"
#include "packaging/InstallationMode.h"
#include "storage/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

namespace zarya {

namespace {

QJsonObject loadBuildIntegrity()
{
    const QString appDir = AppPaths::applicationDir();
    const QStringList candidates = {
        appDir + QStringLiteral("/build-integrity.json"),
#if defined(Q_OS_MACOS)
        QDir(appDir).filePath(QStringLiteral("../Resources/build-integrity.json")),
#endif
    };
    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            return doc.object();
        }
    }
    return {};
}

} // namespace

QString BuildInfo::appVersion()
{
    return QStringLiteral(ZARYA_VERSION_STRING);
}

QString BuildInfo::buildCommit()
{
    return QStringLiteral(ZARYA_BUILD_COMMIT);
}

QString BuildInfo::buildDateUtc()
{
    return QStringLiteral(ZARYA_BUILD_DATE_UTC);
}

QString BuildInfo::buildChannel()
{
    return QStringLiteral(ZARYA_BUILD_CHANNEL);
}

QString BuildInfo::qtVersion()
{
    return QString::fromLatin1(QT_VERSION_STR);
}

QString BuildInfo::compilerInfo()
{
#if defined(_MSC_VER)
    return QStringLiteral("MSVC %1").arg(_MSC_VER);
#elif defined(__clang__)
    return QStringLiteral("Clang %1.%2.%3")
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__);
#elif defined(__GNUC__)
    return QStringLiteral("GCC %1.%2.%3")
        .arg(__GNUC__)
        .arg(__GNUC_MINOR__)
        .arg(__GNUC_PATCHLEVEL__);
#else
    return QStringLiteral("unknown");
#endif
}

bool BuildInfo::isSigned()
{
    return loadBuildIntegrity().value(QStringLiteral("signed")).toBool(false);
}

QString BuildInfo::integrityNote()
{
    const QJsonObject integrity = loadBuildIntegrity();
    const QString note = integrity.value(QStringLiteral("note")).toString();
    if (!note.isEmpty()) {
        return note;
    }
    if (isSigned()) {
        const QString signatureType = integrity.value(QStringLiteral("signatureType")).toString();
        if (!signatureType.isEmpty()) {
            return QStringLiteral("Signed build (%1).").arg(signatureType);
        }
        return QStringLiteral("Signed build.");
    }
    return QStringLiteral(
        "This beta build is unsigned.\n"
        "Use SHA256 checksums from the release page to verify the archive.");
}

QString BuildInfo::cliVersionText()
{
    return QStringLiteral("Zarya %1\nCommit: %2\nBuilt: %3\nChannel: %4\nSigned: %5\nQt: %6")
        .arg(appVersion(),
             buildCommit(),
             buildDateUtc(),
             buildChannel(),
             isSigned() ? QStringLiteral("yes") : QStringLiteral("no"),
             qtVersion());
}

QString BuildInfo::helperCliVersionText()
{
    return QStringLiteral("zarya-helper %1").arg(appVersion());
}

QString BuildInfo::aboutText()
{
    QString text = QStringLiteral(
                       "Zarya %1 (%2)\n"
                       "Build:\n"
                       "  Version: %1\n"
                       "  Channel: %2\n"
                       "  Commit: %3\n"
                       "  Signed: %4\n"
                       "  Installation: %7\n"
                       "Built: %5\n"
                       "Qt %6\n\n"
                       "Cross-platform proxy profile manager with Xray system proxy, "
                       "subscriptions, routing, DNS, backup, and diagnostics.")
                       .arg(appVersion(),
                            buildChannel(),
                            buildCommit(),
                            isSigned() ? QStringLiteral("yes") : QStringLiteral("no"),
                            buildDateUtc(),
                            qtVersion(),
                            InstallationInfo::currentModeString());

    const QString note = integrityNote();
    if (!note.isEmpty()) {
        text += QStringLiteral("\n\n%1").arg(note);
    }
    return text;
}

} // namespace zarya
