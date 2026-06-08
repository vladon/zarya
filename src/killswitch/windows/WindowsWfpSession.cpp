#include "killswitch/windows/WindowsWfpPreamble.h"

#include "killswitch/windows/WindowsWfpSession.h"

#include "killswitch/KillSwitchLog.h"
#include "killswitch/windows/WindowsWfpGuids.h"
#include "killswitch/windows/WindowsWfpUtils.h"

namespace zarya {

WindowsWfpSession::~WindowsWfpSession()
{
    close();
}

bool WindowsWfpSession::open(QString* errorMessage)
{
    if (m_engine != nullptr) {
        return true;
    }

    killSwitchLog(QStringLiteral("WFP backend: opening engine"));
    DWORD result = FwpmEngineOpen0(nullptr, RPC_C_AUTHN_DEFAULT, nullptr, nullptr, &m_engine);
    if (result != ERROR_SUCCESS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        m_engine = nullptr;
        return false;
    }
    return true;
}

void WindowsWfpSession::close()
{
    if (m_inTransaction) {
        abortTransaction();
    }
    if (m_engine != nullptr) {
        FwpmEngineClose0(m_engine);
        m_engine = nullptr;
    }
}

bool WindowsWfpSession::beginTransaction(QString* errorMessage)
{
    if (!m_engine) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("WFP engine is not open.");
        }
        return false;
    }
    const DWORD result = FwpmTransactionBegin0(m_engine, 0);
    if (result != ERROR_SUCCESS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }
    m_inTransaction = true;
    return true;
}

bool WindowsWfpSession::commitTransaction(QString* errorMessage)
{
    if (!m_engine || !m_inTransaction) {
        return true;
    }
    const DWORD result = FwpmTransactionCommit0(m_engine);
    m_inTransaction = false;
    if (result != ERROR_SUCCESS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }
    return true;
}

void WindowsWfpSession::abortTransaction()
{
    if (m_engine != nullptr && m_inTransaction) {
        FwpmTransactionAbort0(m_engine);
        killSwitchLog(QStringLiteral("WFP transaction aborted"));
    }
    m_inTransaction = false;
}

bool WindowsWfpSession::ensureProvider(QString* errorMessage)
{
    FWPM_PROVIDER0* existing = nullptr;
    DWORD result = FwpmProviderGetByKey0(m_engine, &ZARYA_WFP_PROVIDER_KEY, &existing);
    if (result == ERROR_SUCCESS) {
        FwpmFreeMemory0(reinterpret_cast<void**>(&existing));
        killSwitchLog(QStringLiteral("WFP provider ensured"));
        return true;
    }
    if (result != FWP_E_PROVIDER_NOT_FOUND) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }

    FWPM_PROVIDER0 provider = {};
    provider.providerKey = ZARYA_WFP_PROVIDER_KEY;
    provider.displayData.name = const_cast<wchar_t*>(L"Zarya Kill Switch");
    provider.displayData.description =
        const_cast<wchar_t*>(L"Zarya experimental kill switch provider (PoC)");
    provider.flags = 0;

    result = FwpmProviderAdd0(m_engine, &provider, nullptr);
    if (result != ERROR_SUCCESS && result != FWP_E_ALREADY_EXISTS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }
    killSwitchLog(QStringLiteral("WFP provider ensured"));
    return true;
}

bool WindowsWfpSession::ensureSublayer(QString* errorMessage)
{
    FWPM_SUBLAYER0* existing = nullptr;
    DWORD result = FwpmSubLayerGetByKey0(m_engine, &ZARYA_WFP_SUBLAYER_KEY, &existing);
    if (result == ERROR_SUCCESS) {
        FwpmFreeMemory0(reinterpret_cast<void**>(&existing));
        killSwitchLog(QStringLiteral("WFP sublayer ensured"));
        return true;
    }
    if (result != FWP_E_SUBLAYER_NOT_FOUND) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }

    FWPM_SUBLAYER0 sublayer = {};
    GUID providerKey = ZARYA_WFP_PROVIDER_KEY;
    sublayer.subLayerKey = ZARYA_WFP_SUBLAYER_KEY;
    sublayer.displayData.name = const_cast<wchar_t*>(L"Zarya Kill Switch");
    sublayer.displayData.description =
        const_cast<wchar_t*>(L"Zarya experimental kill switch filters (PoC)");
    sublayer.providerKey = &providerKey;
    sublayer.weight = 0x8000;

    result = FwpmSubLayerAdd0(m_engine, &sublayer, nullptr);
    if (result != ERROR_SUCCESS && result != FWP_E_ALREADY_EXISTS) {
        if (errorMessage) {
            *errorMessage = wfpFormatError(result);
        }
        return false;
    }
    killSwitchLog(QStringLiteral("WFP sublayer ensured"));
    return true;
}

bool WindowsWfpSession::deleteZaryaFilters(QString* errorMessage)
{
    killSwitchLog(QStringLiteral("Deleting stale Zarya filters"));
    GUID keys[64] = {};
    int count = 0;
    zaryaAllZaryaFilterKeys(keys, &count);

    for (int index = 0; index < count; ++index) {
        const DWORD result = FwpmFilterDeleteByKey0(m_engine, &keys[index]);
        if (result != ERROR_SUCCESS && result != FWP_E_FILTER_NOT_FOUND) {
            if (errorMessage) {
                *errorMessage = wfpFormatError(result);
            }
            return false;
        }
    }
    return true;
}

} // namespace zarya
