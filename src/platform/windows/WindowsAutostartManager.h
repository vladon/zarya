#pragma once

#include "platform/IAutostartManager.h"

namespace zarya {

class WindowsAutostartManager : public IAutostartManager {
public:
    bool isSupported() const override;
    bool isEnabled(QString* errorMessage = nullptr) const override;
    bool enable(const QStringList& arguments, QString* errorMessage = nullptr) override;
    bool disable(QString* errorMessage = nullptr) override;
    QString backendName() const override;
    QString limitations() const override;

private:
    static QString runKeyPath();
    static QString valueName();
    static QString buildCommandLine(const QStringList& arguments);
};

} // namespace zarya
