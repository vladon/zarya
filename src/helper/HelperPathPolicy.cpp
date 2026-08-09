#include "helper/HelperPathPolicy.h"

#include <QDir>
#include <QFileInfo>

namespace zarya {

void HelperPathPolicy::setAllowedRuntimeDir(const QString& path)
{
    m_allowedRuntimeDir = canonicalDir(path);
}

void HelperPathPolicy::setAllowedCoreDir(const QString& path)
{
    m_allowedCoreDir = canonicalDir(path);
}

void HelperPathPolicy::setAllowedClientSid(const QString& sid)
{
    m_allowedClientSid = sid.trimmed();
}

void HelperPathPolicy::setAllowedClientUid(const QString& uid)
{
    m_allowedClientUid = uid.trimmed();
}

QString HelperPathPolicy::allowedClientSid() const
{
    return m_allowedClientSid;
}

QString HelperPathPolicy::allowedClientUid() const
{
    return m_allowedClientUid;
}

QString HelperPathPolicy::canonicalDir(const QString& path)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }
    return QDir(path.trimmed()).absolutePath();
}

bool HelperPathPolicy::isUnderDir(const QString& filePath, const QString& allowedDir) const
{
    if (allowedDir.isEmpty()) {
        return false;
    }
    const QFileInfo info(filePath);
    const QString canonicalFile = info.isAbsolute() ? info.absoluteFilePath()
                                                    : QDir(allowedDir).absoluteFilePath(filePath);
    const QString canonicalDirPath = QDir(allowedDir).absolutePath();
    return canonicalFile.startsWith(canonicalDirPath + QLatin1Char('/'))
           || canonicalFile.startsWith(canonicalDirPath + QLatin1Char('\\'))
           || canonicalFile.compare(canonicalDirPath, Qt::CaseInsensitive) == 0;
}

bool HelperPathPolicy::isAllowedConfigPath(const QString& configPath, QString* reason) const
{
    const QFileInfo info(configPath);
    if (!info.exists() || !info.isFile()) {
        if (reason) {
            *reason = QStringLiteral("Config file does not exist.");
        }
        return false;
    }
    if (!isUnderDir(configPath, m_allowedRuntimeDir)) {
        if (reason) {
            *reason = QStringLiteral("Config path is outside allowed runtime directory.");
        }
        return false;
    }
    return true;
}

} // namespace zarya
