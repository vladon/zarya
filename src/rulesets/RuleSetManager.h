#pragma once

#include "domain/DnsProfile.h"
#include "domain/RoutingProfile.h"
#include "rulesets/RequiredRuleSetDetector.h"
#include "rulesets/RuleSetItem.h"
#include "runtime/singbox/SingBoxRuleSetContext.h"
#include "storage/RuleSetStore.h"

#include <QObject>
#include <QVector>

namespace zarya {

class RuleSetManager : public QObject {
    Q_OBJECT

public:
    explicit RuleSetManager(QObject* parent = nullptr);

    QString targetDirectory() const;
    bool reload(QString* errorMessage = nullptr);
    QVector<RuleSetItem> items() const;
    RuleSetItem* findItem(const QString& tag);
    const RuleSetItem* findItem(const QString& tag) const;

    QVector<RequiredRuleSet> detectRequired(const RoutingProfile& routingProfile,
                                            const DnsProfile& dnsProfile) const;

    SingBoxRuleSetContext buildContext(const RoutingProfile& routingProfile,
                                       const DnsProfile& dnsProfile, bool requireLocal) const;

    QStringList missingRequiredTags(const RoutingProfile& routingProfile,
                                    const DnsProfile& dnsProfile, bool requireLocal) const;

    bool importLocalSrs(const QString& tag, const QString& sourcePath, QString* errorMessage);
    bool importLocalJson(const QString& tag, const QString& sourcePath, QString* errorMessage);
    bool compileTag(const QString& tag, QString* errorMessage);
    bool downloadItem(const QString& tag, QString* errorMessage);
    bool addCustomItem(const RuleSetItem& item, QString* errorMessage);
    bool removeCustomItem(const QString& tag, QString* errorMessage);

signals:
    void itemsChanged();
    void logLine(const QString& line);

private:
    void refreshItemStatus(RuleSetItem& item);
    bool installDownloadedFile(const QString& downloadPath, const QString& finalPath,
                               const QString& backupPath, const QString& expectedSha256,
                               QString* errorMessage);

    QVector<RuleSetItem> m_items;
    RuleSetStore m_store;
};

} // namespace zarya
