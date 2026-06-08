#pragma once

#include "killswitch/KillSwitchRuleSet.h"
#include "killswitch/KillSwitchState.h"

#include <QJsonObject>
#include <QObject>
#include <memory>

namespace zarya {

struct KillSwitchMarkerData;

class IKillSwitchBackend {
public:
    virtual ~IKillSwitchBackend() = default;

    virtual QString backendId() const = 0;
    virtual QString displayName() const = 0;
    virtual KillSwitchState checkSupport(bool privileged) const = 0;
    virtual bool enable(const KillSwitchRuleSet& rules, QString* errorMessage) = 0;
    virtual bool disable(QString* errorMessage) = 0;
    virtual QString recoveryInstructions() const = 0;
    virtual void augmentMarker(KillSwitchMarkerData* data) const;
    virtual QStringList activeRuleDescriptions() const;
};

class KillSwitchManager : public QObject {
    Q_OBJECT

public:
    explicit KillSwitchManager(QObject* parent = nullptr);

    KillSwitchState state() const;
    void refreshStartupState(bool privileged);

    KillSwitchState checkSupport(bool privileged) const;
    bool enable(const KillSwitchRuleSet& rules, bool privileged, QString* errorMessage);
    bool disable(QString* errorMessage);
    bool recover(bool force, QString* errorMessage);

    static QString recoveryInstructionsForPlatform();
    static KillSwitchRuleSet ruleSetFromJson(const QJsonObject& payload);
    static QJsonObject stateToJson(const KillSwitchState& state);

signals:
    void logLine(const QString& line);
    void stateChanged(const KillSwitchState& state);

private:
    void setState(KillSwitchState state);
    std::unique_ptr<IKillSwitchBackend> createBackend();

    std::unique_ptr<IKillSwitchBackend> m_backend;
    KillSwitchState m_state;
};

} // namespace zarya
