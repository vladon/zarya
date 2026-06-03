#pragma once

#include "platform/IAutostartManager.h"

namespace zarya {

class StubAutostartManager : public IAutostartManager {
public:
    explicit StubAutostartManager(QString reason = {});

    bool isSupported() const override;
    bool isEnabled(QString* errorMessage = nullptr) const override;
    bool enable(const QStringList& arguments, QString* errorMessage = nullptr) override;
    bool disable(QString* errorMessage = nullptr) override;
    QString backendName() const override;
    QString limitations() const override;

private:
    QString m_reason;
};

} // namespace zarya
