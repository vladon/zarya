#pragma once

#include <QString>

namespace zarya {

struct PortableDataPreview {
    QString folder;
    bool valid = false;
    bool hasPortableFlag = false;
    int profileCount = 0;
    int subscriptionCount = 0;
    bool hasRouting = false;
    bool hasDns = false;
    bool hasSettings = false;
};

class PortableMigration {
public:
    static QString portableDataDir(const QString& portableRoot);
    static PortableDataPreview preview(const QString& portableRoot);
    static bool createBackupArchive(const QString& portableRoot, const QString& outputZipPath,
                                    QString* error);
};

} // namespace zarya
