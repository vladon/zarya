#pragma once

#include "platform/IAutostartManager.h"

namespace zarya {

class MacAutostartManager : public IAutostartManager {
public:
    bool isSupported() const override;
    bool isEnabled(QString* errorMessage = nullptr) const override;
    bool enable(const QStringList& arguments, QString* errorMessage = nullptr) override;
    bool disable(QString* errorMessage = nullptr) override;
    QString backendName() const override;
    QString limitations() const override;

private:
    static QString plistPath();
    static QString label();
    static QString executablePath();
    static bool writePlist(const QStringList& arguments, QString* errorMessage);
};

} // namespace zarya
