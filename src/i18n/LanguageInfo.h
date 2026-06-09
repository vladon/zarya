#pragma once

#include <QString>

namespace zarya {

struct LanguageInfo {
    QString code;
    QString nativeName;
    QString englishName;
    bool isBuiltIn = true;
};

} // namespace zarya
