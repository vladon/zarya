#pragma once

#include <QtGlobal>

namespace zarya {

// Keeps a Windows Job Object with JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE so that
// force-killing the parent process also terminates attached children.
// No-op on non-Windows platforms.
class KillOnCloseProcessJob {
public:
    KillOnCloseProcessJob() = default;
    ~KillOnCloseProcessJob();

    KillOnCloseProcessJob(const KillOnCloseProcessJob&) = delete;
    KillOnCloseProcessJob& operator=(const KillOnCloseProcessJob&) = delete;

    // Attach a running child process by PID. Returns false if unsupported or failed.
    bool attach(qint64 pid);
    void reset();
    bool isActive() const;

private:
#if defined(Q_OS_WIN)
    void* m_jobHandle = nullptr;
#endif
};

} // namespace zarya
