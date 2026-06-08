#pragma once

#include "killswitch/windows/WindowsWfpPreamble.h"

#include <QString>

namespace zarya {

class WindowsWfpSession {
public:
    WindowsWfpSession() = default;
    ~WindowsWfpSession();

    WindowsWfpSession(const WindowsWfpSession&) = delete;
    WindowsWfpSession& operator=(const WindowsWfpSession&) = delete;

    bool open(QString* errorMessage);
    void close();

    HANDLE engine() const { return m_engine; }
    bool isOpen() const { return m_engine != nullptr; }

    bool beginTransaction(QString* errorMessage);
    bool commitTransaction(QString* errorMessage);
    void abortTransaction();

    bool ensureProvider(QString* errorMessage);
    bool ensureSublayer(QString* errorMessage);
    bool deleteZaryaFilters(QString* errorMessage);

private:
    HANDLE m_engine = nullptr;
    bool m_inTransaction = false;
};

} // namespace zarya
