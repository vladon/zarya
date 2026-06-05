#include "rulesets/RuleSetManager.h"

#include "packaging/PackagingInfo.h"
#include "rulesets/RuleSetCatalog.h"
#include "rulesets/RuleSetCompiler.h"
#include "rulesets/RuleSetDownloader.h"
#include "rulesets/RuleSetVerifier.h"
#include "storage/AppPaths.h"
#include "storage/AppSettings.h"
#include <QDir>
#include <algorithm>
#include <QFile>
#include <QFileInfo>

namespace zarya {

RuleSetManager::RuleSetManager(QObject* parent)
    : QObject(parent)
{
    reload(nullptr);
}

QString RuleSetManager::targetDirectory() const
{
    return AppPaths::singBoxRuleSetDir();
}

bool RuleSetManager::reload(QString* errorMessage)
{
    m_items = RuleSetCatalog::builtInItems();
    QVector<RuleSetItem> customItems;
    if (!m_store.load(&customItems, errorMessage)) {
        return false;
    }
    for (const RuleSetItem& custom : customItems) {
        if (RuleSetItem* existing = RuleSetCatalog::findByTag(m_items, custom.tag)) {
            *existing = custom;
            existing->builtIn = false;
        } else {
            m_items.append(custom);
        }
    }
    for (RuleSetItem& item : m_items) {
        refreshItemStatus(item);
    }
    emit itemsChanged();
    return true;
}

QVector<RuleSetItem> RuleSetManager::items() const
{
    return m_items;
}

RuleSetItem* RuleSetManager::findItem(const QString& tag)
{
    return RuleSetCatalog::findByTag(m_items, tag);
}

const RuleSetItem* RuleSetManager::findItem(const QString& tag) const
{
    return RuleSetCatalog::findByTag(m_items, tag);
}

void RuleSetManager::refreshItemStatus(RuleSetItem& item)
{
    item.localSrsPath = AppPaths::localRuleSetSrsPath(item.tag);
    item.localJsonPath = AppPaths::localRuleSetJsonPath(item.tag);
    if (QFile::exists(item.localSrsPath)) {
        const QFileInfo info(item.localSrsPath);
        item.sizeBytes = info.size();
        item.modifiedAt = info.lastModified();
        item.sha256 = RuleSetVerifier::sha256OfFile(item.localSrsPath);
        item.status = RuleSetStatus::Present;
        if (!item.expectedSha256.isEmpty()
            && item.sha256.compare(item.expectedSha256, Qt::CaseInsensitive) == 0) {
            item.status = RuleSetStatus::Verified;
        }
        item.lastError.clear();
        return;
    }

    item.sizeBytes = 0;
    item.modifiedAt = {};
    item.sha256.clear();
    if (item.builtIn && !item.srsUrl.isValid() && !item.jsonUrl.isValid()) {
        item.status = RuleSetStatus::SourceMissing;
        item.lastError = QStringLiteral("No built-in download URL configured yet.");
    } else if (!item.srsUrl.isValid() && !item.jsonUrl.isValid()) {
        item.status = RuleSetStatus::Unsupported;
    } else {
        item.status = RuleSetStatus::Missing;
    }
}

QVector<RequiredRuleSet> RuleSetManager::detectRequired(const RoutingProfile& routingProfile,
                                                        const DnsProfile& dnsProfile) const
{
    QVector<RequiredRuleSet> required =
        RequiredRuleSetDetector::detect(routingProfile, dnsProfile);
    for (RequiredRuleSet& entry : required) {
        if (const RuleSetItem* item = RuleSetCatalog::findByTag(m_items, entry.tag)) {
            entry.available = item->status == RuleSetStatus::Present
                              || item->status == RuleSetStatus::Verified;
            entry.localPath = item->localSrsPath;
            entry.catalogStatus = item->status;
            if (!entry.available) {
                entry.warning = QStringLiteral("Required rule set %1 is %2.")
                                    .arg(entry.tag, ruleSetStatusDisplayName(item->status));
            }
        }
    }
    return required;
}

SingBoxRuleSetContext RuleSetManager::buildContext(const RoutingProfile& routingProfile,
                                                   const DnsProfile& dnsProfile,
                                                   bool requireLocal) const
{
    SingBoxRuleSetContext context;
    context.requireLocalRuleSets = requireLocal;
    context.useRuleSetReferences = true;
    for (const RequiredRuleSet& required : detectRequired(routingProfile, dnsProfile)) {
        if (required.available) {
            context.tagToLocalPath.insert(required.tag, required.localPath);
        }
    }
    return context;
}

QStringList RuleSetManager::missingRequiredTags(const RoutingProfile& routingProfile,
                                                const DnsProfile& dnsProfile,
                                                bool requireLocal) const
{
    QStringList missing;
    if (!requireLocal) {
        return missing;
    }
    for (const RequiredRuleSet& required : detectRequired(routingProfile, dnsProfile)) {
        if (!required.available) {
            missing.append(required.tag);
        }
    }
    return missing;
}

bool RuleSetManager::importLocalSrs(const QString& tag, const QString& sourcePath,
                                      QString* errorMessage)
{
    if (tag.trimmed().isEmpty() || !QFile::exists(sourcePath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid tag or source file.");
        }
        return false;
    }
    const QString destination = AppPaths::localRuleSetSrsPath(tag);
    QDir().mkpath(QFileInfo(destination).absolutePath());
    if (QFile::exists(destination)) {
        QFile::remove(destination + QStringLiteral(".bak"));
        QFile::rename(destination, destination + QStringLiteral(".bak"));
    }
    if (!QFile::copy(sourcePath, destination)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not copy rule set to %1").arg(destination);
        }
        return false;
    }
    emit logLine(QStringLiteral("Imported rule set %1").arg(tag));
    reload(errorMessage);
    return true;
}

bool RuleSetManager::importLocalJson(const QString& tag, const QString& sourcePath,
                                     QString* errorMessage)
{
    const QString jsonPath = AppPaths::localRuleSetJsonPath(tag);
    QDir().mkpath(QFileInfo(jsonPath).absolutePath());
    if (QFile::exists(jsonPath)) {
        QFile::remove(jsonPath);
    }
    if (!QFile::copy(sourcePath, jsonPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not copy JSON source.");
        }
        return false;
    }
    return compileTag(tag, errorMessage);
}

bool RuleSetManager::compileTag(const QString& tag, QString* errorMessage)
{
    const QString jsonPath = AppPaths::localRuleSetJsonPath(tag);
    if (!QFile::exists(jsonPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Source JSON not found for %1.").arg(tag);
        }
        return false;
    }
    RuleSetCompiler compiler;
    const RuleSetCompileResult result = compiler.compileJsonToSrs(
        AppSettings::instance().resolvedSingBoxPath(), jsonPath,
        AppPaths::localRuleSetSrsPath(tag));
    if (!result.output.trimmed().isEmpty()) {
        emit logLine(result.output.trimmed());
    }
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = result.errorMessage;
        }
        if (RuleSetItem* item = findItem(tag)) {
            item->status = RuleSetStatus::CompileFailed;
            item->lastError = result.errorMessage;
        }
        return false;
    }
    emit logLine(QStringLiteral("Compiled rule set %1").arg(tag));
    reload(errorMessage);
    return true;
}

bool RuleSetManager::installDownloadedFile(const QString& downloadPath, const QString& finalPath,
                                           const QString& backupPath, const QString& expectedSha256,
                                           QString* errorMessage)
{
    if (!RuleSetVerifier::verifyFileSha256(downloadPath, expectedSha256, errorMessage)) {
        QFile::remove(downloadPath);
        return false;
    }
    if (QFile::exists(finalPath)) {
        QFile::remove(backupPath);
        QFile::rename(finalPath, backupPath);
    }
    if (!QFile::rename(downloadPath, finalPath)) {
        if (QFile::exists(backupPath)) {
            QFile::rename(backupPath, finalPath);
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not install downloaded rule set.");
        }
        return false;
    }
    return true;
}

bool RuleSetManager::downloadItem(const QString& tag, QString* errorMessage)
{
    RuleSetItem* item = findItem(tag);
    if (!item) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown rule set: %1").arg(tag);
        }
        return false;
    }

    item->status = RuleSetStatus::Updating;
    const QString userAgent =
        QStringLiteral("Zarya/%1").arg(PackagingInfo::versionString());
    RuleSetDownloader downloader;

    if (item->jsonUrl.isValid() && !item->srsUrl.isValid()) {
        const QString jsonDownload = item->localJsonPath + QStringLiteral(".download");
        if (!downloader.downloadToFile(item->jsonUrl, jsonDownload, userAgent, errorMessage)) {
            item->status = RuleSetStatus::DownloadFailed;
            item->lastError = errorMessage ? *errorMessage : QString();
            return false;
        }
        if (QFile::exists(item->localJsonPath)) {
            QFile::remove(item->localJsonPath);
        }
        QFile::rename(jsonDownload, item->localJsonPath);
        return compileTag(tag, errorMessage);
    }

    if (!item->srsUrl.isValid()) {
        item->status = RuleSetStatus::SourceMissing;
        if (errorMessage) {
            *errorMessage = QStringLiteral("No download URL configured for %1.").arg(tag);
        }
        return false;
    }

    const QString downloadPath = item->localSrsPath + QStringLiteral(".download");
    if (!downloader.downloadToFile(item->srsUrl, downloadPath, userAgent, errorMessage)) {
        item->status = RuleSetStatus::DownloadFailed;
        item->lastError = errorMessage ? *errorMessage : QString();
        QFile::remove(downloadPath);
        return false;
    }

    if (!item->checksumUrl.isEmpty()) {
        const QString checksumDownload = downloadPath + QStringLiteral(".sha256");
        if (downloader.downloadToFile(item->checksumUrl, checksumDownload, userAgent,
                                      errorMessage)) {
            QFile checksumFile(checksumDownload);
            if (checksumFile.open(QIODevice::ReadOnly)) {
                item->expectedSha256 = QString::fromUtf8(checksumFile.readAll()).trimmed();
            }
            checksumFile.remove();
        }
    }

    if (!installDownloadedFile(downloadPath, item->localSrsPath, item->localSrsPath + QStringLiteral(".bak"),
                             item->expectedSha256, errorMessage)) {
        item->status = RuleSetStatus::ChecksumFailed;
        item->lastError = errorMessage ? *errorMessage : QString();
        return false;
    }

    emit logLine(QStringLiteral("Downloaded rule set %1").arg(tag));
    reload(errorMessage);
    return true;
}

bool RuleSetManager::addCustomItem(const RuleSetItem& item, QString* errorMessage)
{
    QVector<RuleSetItem> custom;
    if (!m_store.load(&custom, errorMessage)) {
        return false;
    }
    if (RuleSetItem* existing = RuleSetCatalog::findByTag(custom, item.tag)) {
        *existing = item;
    } else {
        custom.append(item);
    }
    if (!m_store.save(custom, errorMessage)) {
        return false;
    }
    return reload(errorMessage);
}

bool RuleSetManager::removeCustomItem(const QString& tag, QString* errorMessage)
{
    QVector<RuleSetItem> custom;
    if (!m_store.load(&custom, errorMessage)) {
        return false;
    }
    custom.erase(std::remove_if(custom.begin(), custom.end(),
                                [&tag](const RuleSetItem& item) { return item.tag == tag; }),
                 custom.end());
    if (!m_store.save(custom, errorMessage)) {
        return false;
    }
    return reload(errorMessage);
}

} // namespace zarya
