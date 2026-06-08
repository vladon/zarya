#pragma once

#include "cores/CoreReleaseProvider.h"

namespace zarya {

class GitHubReleaseProvider : public CoreReleaseProvider {
    Q_OBJECT

public:
    explicit GitHubReleaseProvider(CoreType type, const QString& repoSlug,
                                   QObject* parent = nullptr);

    CoreType coreType() const override;
    QString providerName() const override;
    void fetchLatestRelease(int timeoutMs) override;

private:
    CoreType m_type;
    QString m_repoSlug;
};

} // namespace zarya
