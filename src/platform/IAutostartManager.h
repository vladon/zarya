#pragma once

#include <QString>
#include <QStringList>

namespace zarya {

class IAutostartManager {
public:
    virtual ~IAutostartManager() = default;

    virtual bool isSupported() const = 0;
    virtual bool isEnabled(QString* errorMessage = nullptr) const = 0;
    virtual bool enable(const QStringList& arguments, QString* errorMessage = nullptr) = 0;
    virtual bool disable(QString* errorMessage = nullptr) = 0;
    virtual QString backendName() const = 0;
    virtual QString limitations() const = 0;
};

} // namespace zarya
