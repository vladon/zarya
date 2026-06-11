#pragma once

#include <QString>

namespace zarya {

class PublicBetaDocs {
public:
    static QString bundledDocPath(const QString& fileName);
    static bool openBundledDoc(const QString& fileName);
    static bool openIssueReporting();
    static QString projectIssuesUrl();
};

} // namespace zarya
