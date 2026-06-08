#include "killswitch/windows/WindowsWfpPreamble.h"

#include "killswitch/windows/WindowsWfpKillSwitchManager.h"

#include "killswitch/KillSwitchLog.h"
#include "killswitch/KillSwitchMarker.h"
#include "killswitch/windows/WindowsWfpFilterBuilder.h"
#include "killswitch/windows/WindowsWfpGuids.h"
#include "killswitch/windows/WindowsWfpSession.h"
#include "killswitch/windows/WindowsWfpUtils.h"
namespace zarya {

QString WindowsWfpKillSwitchManager::backendId() const
{
    return QStringLiteral("windows-wfp");
}

QString WindowsWfpKillSwitchManager::displayName() const
{
    return QStringLiteral("Windows WFP PoC");
}

KillSwitchState WindowsWfpKillSwitchManager::checkSupport(bool privileged) const
{
    KillSwitchState state;
    state.backend = backendId();
    state.privileged = privileged;
    state.supported = false;
    state.status = KillSwitchStatus::Unsupported;

    if (!privileged) {
        state.lastError = QStringLiteral(
            "zarya-helper must run as Administrator to manage WFP filters");
        return state;
    }

    QString bfeError;
    if (!wfpIsBfeServiceRunning(&bfeError)) {
        state.lastError = bfeError;
        return state;
    }

    WindowsWfpSession session;
    QString openError;
    if (!session.open(&openError)) {
        state.lastError = openError;
        return state;
    }

    state.status = KillSwitchStatus::Disabled;
    state.supported = true;
    state.lastError.clear();
    return state;
}

bool WindowsWfpKillSwitchManager::enable(const KillSwitchRuleSet& rules, QString* errorMessage)
{
    WindowsWfpSession session;
    QString error;
    if (!session.open(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        killSwitchLog(QStringLiteral("WFP transaction aborted: %1").arg(error));
        return false;
    }

    if (!session.beginTransaction(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        killSwitchLog(QStringLiteral("WFP transaction aborted: %1").arg(error));
        return false;
    }

    auto rollback = [&]() {
        session.abortTransaction();
        killSwitchLog(QStringLiteral("WFP transaction aborted: %1").arg(error));
    };

    if (!session.ensureProvider(&error) || !session.ensureSublayer(&error)
        || !session.deleteZaryaFilters(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        rollback();
        return false;
    }

    if (!WindowsWfpFilterBuilder::addLoopbackAllows(session, rules.allowLoopback, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        rollback();
        return false;
    }

    QStringList warnings;
    if (!WindowsWfpFilterBuilder::addProxyAllows(session, rules, &warnings, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        rollback();
        return false;
    }
    for (const QString& warning : warnings) {
        killSwitchLog(warning);
    }

    WindowsWfpTunInterfaceInfo tunInfo;
    QString tunWarning;
    WindowsWfpFilterBuilder::findTunInterface(rules.tunInterfaceName, &tunInfo, &tunWarning);
    if (!WindowsWfpFilterBuilder::addTunInterfaceAllows(session, tunInfo, &tunWarning, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        rollback();
        return false;
    }

    if (!WindowsWfpFilterBuilder::addOutboundBlocks(session, &error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        rollback();
        return false;
    }

    if (!session.commitTransaction(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        session.abortTransaction();
        killSwitchLog(QStringLiteral("WFP transaction aborted: %1").arg(error));
        return false;
    }

    killSwitchLog(QStringLiteral("WFP kill switch enabled"));
    return true;
}

bool WindowsWfpKillSwitchManager::disable(QString* errorMessage)
{
    killSwitchLog(QStringLiteral("Deleting Zarya WFP filters"));

    WindowsWfpSession session;
    QString error;
    if (!session.open(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    if (!session.beginTransaction(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        return false;
    }

    if (!session.deleteZaryaFilters(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        session.abortTransaction();
        return false;
    }

    if (!session.commitTransaction(&error)) {
        if (errorMessage) {
            *errorMessage = error;
        }
        session.abortTransaction();
        return false;
    }

    killSwitchLog(QStringLiteral("WFP kill switch disabled"));
    return true;
}

QString WindowsWfpKillSwitchManager::recoveryInstructions() const
{
    return QStringLiteral(
        "Windows WFP kill switch recovery:\n\n"
        "Preferred:\n"
        "  zarya-helper --recover-killswitch\n\n"
        "From Zarya GUI:\n"
        "  Settings → Kill Switch → Disable Now\n\n"
        "Manual inspection:\n"
        "  netsh wfp show state\n\n"
        "Zarya only removes its own provider/sublayer filters. Do not delete unrelated WFP "
        "objects.");
}

void WindowsWfpKillSwitchManager::augmentMarker(KillSwitchMarkerData* data) const
{
    if (!data) {
        return;
    }
    data->providerKey = wfpGuidToString(ZARYA_WFP_PROVIDER_KEY);
    data->sublayerKey = wfpGuidToString(ZARYA_WFP_SUBLAYER_KEY);

    GUID keys[64] = {};
    int count = 0;
    zaryaAllZaryaFilterKeys(keys, &count);
    for (int index = 0; index < count; ++index) {
        data->filterKeys.append(wfpGuidToString(keys[index]));
    }
}

QStringList WindowsWfpKillSwitchManager::activeRuleDescriptions() const
{
    return WindowsWfpFilterBuilder::activeRuleDescriptions();
}

} // namespace zarya
