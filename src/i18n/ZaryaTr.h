#pragma once

#include <QCoreApplication>
#include <QString>

namespace zarya {

class ZaryaTr {
    Q_DECLARE_TR_FUNCTIONS(ZaryaTr)

public:
    static QString plural(const char* sourceText, int count);
};

} // namespace zarya
