#include "i18n/ZaryaTr.h"

#include <QCoreApplication>

namespace zarya {

QString ZaryaTr::plural(const char* sourceText, int count)
{
    return QCoreApplication::translate("Zarya", sourceText, nullptr, count);
}

} // namespace zarya
