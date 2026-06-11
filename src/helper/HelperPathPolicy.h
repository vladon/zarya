#pragma once

#include <QString>

namespace zarya {

class HelperPathPolicy {
public:
    void setAllowedRuntimeDir(const QString& path);
    void setAllowedCoreDir(const QString& path);
    void setAllowedClientSid(const QString& sid);
    void setAllowedClientUid(const QString& uid);

    QString allowedClientSid() const;
    QString allowedClientUid() const;

    bool isAllowedConfigPath(const QString& configPath, QString* reason = nullptr) const;
    bool isAllowedSingBoxPath(const QString& singBoxPath, QString* reason = nullptr) const;
    bool isAllowedWorkingDirectory(const QString& workingDirectory,
                                   const QString& singBoxPath,
                                   QString* reason = nullptr) const;

private:
    static QString canonicalDir(const QString& path);
    bool isUnderDir(const QString& filePath, const QString& allowedDir) const;

    QString m_allowedRuntimeDir;
    QString m_allowedCoreDir;
    QString m_allowedClientSid;
    QString m_allowedClientUid;
};

} // namespace zarya
