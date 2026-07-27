#include "platform/KillOnCloseProcessJob.h"

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace zarya {

KillOnCloseProcessJob::~KillOnCloseProcessJob()
{
    reset();
}

bool KillOnCloseProcessJob::isActive() const
{
#if defined(Q_OS_WIN)
    return m_jobHandle != nullptr;
#else
    return false;
#endif
}

void KillOnCloseProcessJob::reset()
{
#if defined(Q_OS_WIN)
    if (m_jobHandle) {
        CloseHandle(static_cast<HANDLE>(m_jobHandle));
        m_jobHandle = nullptr;
    }
#endif
}

bool KillOnCloseProcessJob::attach(qint64 pid)
{
    reset();
    if (pid <= 0) {
        return false;
    }

#if defined(Q_OS_WIN)
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) {
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        CloseHandle(job);
        return false;
    }

    HANDLE process = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
                                 FALSE, static_cast<DWORD>(pid));
    if (!process) {
        CloseHandle(job);
        return false;
    }

    const BOOL assigned = AssignProcessToJobObject(job, process);
    CloseHandle(process);
    if (!assigned) {
        CloseHandle(job);
        return false;
    }

    m_jobHandle = job;
    return true;
#else
    Q_UNUSED(pid);
    return false;
#endif
}

} // namespace zarya
